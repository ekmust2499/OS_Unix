#include <stdio.h> 
#include <stdlib.h> 
#include <stdbool.h> 
#include <string.h>  
#include <errno.h>  
#include <syslog.h>  
#include <unistd.h>  
#include <signal.h>  
#include <sys/wait.h>

#define MAXPROC 50
#define NAME_LEN 50
#define ARGS_NUM 50
#define MAX_TRY 50

int MAX_FILE_NAME_LEN = 5 + NAME_LEN + MAXPROC;

char name[MAXPROC][NAME_LEN];
char *args[MAXPROC][ARGS_NUM + 1];  // +1 для null
bool respawnable[MAXPROC];

pid_t pid_list[MAXPROC]; //массив статусов дочерних процессов
int pid_list_tries[MAXPROC]; //массив попыток запуска каждого дочернего процесса
int pid_count = 0; //число процессов

int number_of_command; //количество исполняемых файлов в конфиге
char *name_config_file;

//Функция получения имени команды (/bin/ls -> ls)
char* get_name_command(char* path) {
    char *slash = rindex(path, '/');

    if(slash == NULL) {
        return path;
    } else {
        char *name_command = strdup(slash+1);
        return name_command;
    }
}

//Функция создания файла для каждого порожденного дочернего процесса
//название файла в виде "tmp/sleep-0.pid"
void create_file_for_child(int proc_num) {
    char file_name[MAX_FILE_NAME_LEN];  	
    sprintf(file_name, "/tmp/%s-%d.pid", get_name_command(name[proc_num]), proc_num);

    FILE *fp = fopen(file_name, "wa");

    if (fp == NULL) {
        syslog(LOG_WARNING, "Ошибка открытия файла %s!\n", file_name);
        exit(1);
    }
    //syslog(LOG_INFO, "Файл %s создан!\n", file_name);
    fprintf(fp, "%d", pid_list[proc_num]);
    fclose(fp);
}

void spawn_child_process(int proc_num) {
    int cpid = fork();

    switch (cpid) {
        case -1:
            syslog(LOG_WARNING, "При вызове fork() возникла ошибка; cpid == -1\n");
            exit(1);
        case 0: //породили дочерний процесс
            cpid = getpid();
            syslog(LOG_INFO, "Дочерний процесс для исполняемого файла №=%d с pid=%d порождён!\n", proc_num, cpid);
            //printf("%s/n", name[proc_num]);
            create_file_for_child(proc_num);
            execv(name[proc_num], args[proc_num]);
            if (errno != 0) {
              syslog(LOG_WARNING, "Ошибка в запуске процесса %s !", name[proc_num]);
              exit(1);
            }      
            exit(0);
        default: //в родительском процессе
            pid_list[proc_num] = cpid; 
            pid_count++;
    }
}

//Функция парсинга конфигурационного файла построчно
int parse_file() {
    char str[150];
    char *estr;
    int proc_num = 0;
    int arg_num = 0; 

    FILE *fp = fopen(name_config_file, "r");
	
    if (fp == NULL) {
        syslog(LOG_WARNING, "Ошибка открытия файла!\n");
        exit(1);
    }
	
   while (1)
   {
        estr = fgets (str,sizeof(str),fp);

        if (estr == NULL)
        {
            if ( feof (fp) != 0)
            {                
               break; //закончили читать файл
            }
            else
            {
               syslog(LOG_WARNING, "Ошибка чтения из файла\n");
               break;
            }
        }
		
        if (strcmp(str, "\n") == 0 || strcmp(str, "") == 0) {
            continue;
        }
	    //разбиваем строку по пробелам
	    char* token = strtok(str, " ");

        while (token != NULL) {
		
		    if (strcmp(token, "wait") == 0 || strcmp(token, "wait\n") == 0) {
                respawnable[proc_num] = false;
                args[proc_num][arg_num] = NULL;
                token = strtok(NULL, " ");
                continue;
            }
			
            if (strcmp(token, "respawn") == 0 || strcmp(token, "respawn\n") == 0) {
                respawnable[proc_num] = true;
                args[proc_num][arg_num] = NULL;
                token = strtok(NULL, " ");
                continue;
            }
			
            if (arg_num > 0) { //считываем параметры для передачи исполняемому файлу
                args[proc_num][arg_num] = (char *) malloc(ARGS_NUM);
                strcpy(args[proc_num][arg_num], token);
                arg_num++;				
            }
		
            if (arg_num == 0) {
                strcpy(name[proc_num], token);  //имя исполняемого файла (он же первый параметр)
                args[proc_num][arg_num] = (char *) malloc(strlen(name[proc_num]));
                strcpy(args[proc_num][0], name[proc_num]);
                arg_num++;
            }
            token = strtok(NULL, " ");
        }

	    proc_num++;
	    arg_num= 0;
    } 
	
    fclose(fp);
    return proc_num;
}

//Функция удаления файла для каждого порожденного дочернего процесса
void remove_file_for_child(int proc_num) {
    char file_name[MAX_FILE_NAME_LEN];
    sprintf(file_name, "/tmp/%s-%d.pid", get_name_command(name[proc_num]), proc_num);

    int i = remove(file_name);

    if (i == -1) {
        syslog(LOG_WARNING, "Ошибка удаления файла %s!\n", file_name);
        exit(1);
    }
	//syslog(LOG_INFO, "Файл %s удалён!\n", file_name);
}

void run_init() {
    number_of_command = parse_file();

    //для каждого исполняемого файла порождаем процесс
    for (int i = 0; i < number_of_command; i++) {
        spawn_child_process(i);
        pid_list_tries[i] = 1;
    }

    while (pid_count) {
        int status;
        pid_t cpid = waitpid(-1, &status, 0); //завершился какой-то дочерний процесс

        for (int i = 0; i < number_of_command; i++) {
            if (pid_list[i] == cpid) {			
                syslog(LOG_NOTICE, "Дочерний процесс для исполняемого файла №=%d с pid=%d завершён!\n", i, cpid);
                remove_file_for_child(i);
                pid_list[i]=0;
                pid_count--;
				
                if (status == 0) {  //дочерний процесс завершился успешно
                    if (respawnable[i]) {
                        respawnable[i] = false;
                        spawn_child_process(i); //запускаем процесс повторно для respawn
                        if (pid_list_tries[i] > 1) //если последний запуск был успешен, то сбросить счётчик попыток
                        {
                            pid_list_tries[i] = 1;
                        }						
                    }
                }
                else {  //дочерний процесс завершился с ошибкой -> пробуем запустить повторно
                    if (pid_list_tries[i] < MAX_TRY) {
                        pid_list_tries[i]++;
                        spawn_child_process(i);
                    }
                    else {
                        syslog(LOG_WARNING, "!!! Ошибка в запуске исполняемого файла №=%d %s !!!\n", i, name[i]);	
                    }
                }		
            }
        }
    }
}

//Обработчик сигнала SIGHUP
void sighup_handler() {
    syslog(LOG_INFO, "Получен сигнал -HUP, перезапуск.\n"); 
	
    for (int i = 0; i < number_of_command; i++) {
        if (pid_list[i] > 0) {
            kill(pid_list[i], SIGKILL);  //завершаем все дочерние процессы 
        }
    }
    run_init();
}

//Запускаем программу как демон
void run_as_daemon() {
    int pid = fork();

    switch (pid) {
        case 0:
            setsid();
            chdir("/");
            close((long)stdin);
            close((long)stdout);
            close((long)stderr);
            run_init();
            exit(0);
        case -1:
            syslog(LOG_WARNING, "При вызове fork() возникла ошибка!\n");
            exit(1);        
        default:
            syslog(LOG_INFO, "Демон с pid=%d создан.\n", pid);           
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        syslog(LOG_WARNING, "Формат ввода: ./init config.txt\n");
        exit(1);
    }
	
    name_config_file = argv[1];
    signal(SIGHUP, sighup_handler);
    run_as_daemon();
    return 0; 
}