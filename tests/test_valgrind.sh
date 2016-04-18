for f in `ls ./modfiles/*.mod`
do
    for target in cpu gpu
    do
        logfile=$f.$target.log
        printf "testing %30s::%4s : " $f $target
        valgrind --leak-check=full ../bin/modcc $f -t $target -o tmp.h &> $logfile
        grep "in use at exit" $logfile | awk '{if($6==0){print("success")}else{print("fail")}}'
    done
done

