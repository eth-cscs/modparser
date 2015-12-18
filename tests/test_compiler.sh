#for f in `ls ./modfiles/*.mod`
for f in `ls ./modfiles/conductance/*.mod`
do
    for target in cpu #gpu cyme
    do
        logfile=$f.$target.log
        #printf "testing %-35s : %-4s : " $f $target
        ../bin/modcc $f -t $target -o tmp.h
    done
done
rm -f tmp.h

