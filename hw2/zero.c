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

    char r_buf[SIZE];
    char w_buf[SIZE];
    int read_chuck;
    int r_counter;
    int w_counter = 0;
    int zero_off= 0;
    bool start_zero = false; 

    while (true) {
        read_chuck = read(STDIN_FILENO, r_buf, SIZE);
        r_counter = 0;

        if (read_chuck) {
            while (r_counter <  read_chuck) {
                if (start_zero == false && r_buf[r_counter] != 0) {
                    w_buf[w_counter] = r_buf[r_counter];
                    r_counter++;
                    w_counter++;                    
                }
                else if (start_zero == false && r_buf[r_counter] == 0) {
                    start_zero = true;
                }

                if (w_counter > 0) {
                    write(fd, w_buf, w_counter);
                    w_counter= 0;
                }

                if (start_zero == true && r_buf[r_counter] == 0) {
                    r_counter++;
                    zero_off++;                 
                }
                else if (start_zero == true && r_buf[r_counter] != 0){
                    start_zero = false;
                }

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