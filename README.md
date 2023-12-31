## დავალების ატვირთვა
დავალება უნდა ატვირთოთ თქვენს პერსონალურ Github Classroom-ის რეპოზიტორიაში.

## კომპილაცია

```sh
make
```

## ტესტირება
ჯამში ნამუშევარია მოწმდება 7 ტესტზე (t1-იდან t7-ის ჩათვლით).  
თითოეული ტესტის გაშვებისას -w პარამეტრის დაყენებით შეგიძლიათ აკონტროლოთ თუ რამდენ ნაკადიან გარემოში შემოწმდება ნამუშევარი.  
მაგალითად პირველი ტესტის ოთხ ნაკადიან გარემოში გასაშვებად უნდა გამოიყენოთ:
```sh
./bankdriver -t1 -w4
```
-b პარამეტრის გამოყენებით შეგიძლიათ ჩართოთ ბალანსების შემოწმების ფუნქციონალი:
```sh
./bankdriver -t7 -w4 -b
```
-y პარამეტრის დაყენებით შეგიძლიათ აკონტროლოთ თუ რა ალბათობით გაითიშნონ ნაკადები, მაგალითად -y50 ნიშნავს 50%-იან შანსს:
```sh
./bankdriver -t3 -w2 -y50
```
valgrind --tool=helgrind ის გამოყენებით შეგიძლიათ შეამოწმოთ თქვენი ნამუშევარი race-condition და სხვა ნაკადებთან დაკავშირებულ პრობლემებზე:
```sh
valgrind --tool=helgrind ./bankdriver -r -t1 -w4
valgrind --tool=helgrind ./bankdriver -r -t2 -w4
valgrind --tool=helgrind ./bankdriver -r -t3 -w4
valgrind --tool=helgrind ./bankdriver -r -t4 -w4
valgrind --tool=helgrind ./bankdriver -r -t5 -w4
valgrind --tool=helgrind ./bankdriver -r -t6 -w4
valgrind --tool=helgrind ./bankdriver -r -t7 -w4
```
./bankdriver -t1 -w4 -b
./bankdriver -t2 -w8 -b
./bankdriver -t3 -w4 -b
./bankdriver -t4 -w4 -b
./bankdriver -t5 -w4 -b
./bankdriver -t6 -w4 -b
./bankdriver -t7 -w4 -b