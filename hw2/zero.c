#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdbool.h>

const int SIZE = 512;

int main(int argc, char *argv[]) {
    if (argc == 1) {
        printf("Формат ввода: gzip -cd sparse_file.gz | ./zero new_sparse_file");
        return 1;
    }

    int fd = open(argv[1], O_WRONLY | O_CREAT | O_TRUNC, 0660);

    if (fd < 0) {
        return fd;
    }
	
    
    char r_buf[SIZE];            //буфер для чтения
    char w_buf[SIZE];            //буфер для записи
    int read_chuck;              //прочитанный блок
    int r_counter;               //счётчик чтения
    int w_counter = 0;           //счётчик записи
    int zero_off= 0;             //смещение от начала цепочки нулей
    bool start_zero = false;     //флаг начала чтения цепочки нулей

    while (true) {
        read_chuck = read(STDIN_FILENO, r_buf, SIZE);
        r_counter = 0;

        if (read_chuck) {
            while (r_counter <  read_chuck) {
                //запись ненулевых символов в новый буфер
                if (start_zero == false && r_buf[r_counter] != 0) {
                    w_buf[w_counter] = r_buf[r_counter];
                    r_counter++;
                    w_counter++;                    
                }
                //началась цепочка нулей
                else if (start_zero == false && r_buf[r_counter] == 0) {
                    start_zero = true;
                }
                //запись ненулевых символов из буфера в файл
                if (w_counter > 0) {
                    write(fd, w_buf, w_counter);
                    w_counter= 0;
                }
                //подсчет длины цепочки нулей
                if (start_zero == true && r_buf[r_counter] == 0) {
                    r_counter++;
                    zero_off++;                 
                }
                //закончилась цепочка нулей
                else if (start_zero == true && r_buf[r_counter] != 0){
                    start_zero = false;
                }
                //смещение на нужное число байт при записи в файл
                if (zero_off > 0) {
                    lseek(fd, zero_off, SEEK_CUR);
                    zero_off= 0;
                }
            }
        } 
        else {
            break;
        }
    }

    close(fd);
    return 0;
}