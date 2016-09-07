#for f in `ls ./modfiles/*.mod`
prefix=$(dirname $0)
echo $prefix
for f in `ls $prefix/modfiles/conductance/*.mod`
do
    for target in cpu #gpu cyme
    do
        logfile=$f.$target.log
        #printf "testing %-35s : %-4s : " $f $target
        $prefix/../bin/modcc $f -t $target -o tmp.h
    done
done
rm -f tmp.h
