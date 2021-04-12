# Дополнение к gzip

Автор: Мустафина Екатерина


## Запуск 
* gcc zero.c -o zero
* gzip-cd sparse_file.gz | ./zero new_sparse_file

## Принцип работы
* На вход подается распакованный на стандартный вывод gzip’ом разрежённый файл 
* Программа должна отслеживает цепочку нулей, запоминает их и делает операцию seek на нужное число байт при записи на диск.