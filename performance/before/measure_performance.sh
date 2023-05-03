#! /bin/bash

project_path=../parallel-kmeans

program_prefix=$1

executable=$project_path/${program_prefix}_main
input_file=$project_path/Image_data/color17695.bin

out_file="data_$program_prefix.csv"
> $out_file

threshold=0.001
cluster_values=(2 8 32 128 512 2048 8192)

line=""
for i in "${cluster_values[@]}"
do
    line="$line$i,"
done
echo $line >> $out_file

if [ $2 = "parallel" ]
then
    proc_values=(2 3 4 5 6 7 8)
else
    proc_values=1
fi

for processors in "${proc_values[@]}"
do  
    line=""
    for clusters in "${cluster_values[@]}"
    do
        time_sum=0
        for trial in {1..5}
        do
            if [ $2 = "parallel" ]
            then
                results=$($executable -o -bi $input_file -n $clusters -t $threshold -p $processors)
            else
                results=$($executable -o -bi $input_file -n $clusters -t $threshold )
            fi
            time_lines=$(echo "$results" | sed -n '12, 13p')
            # io_time=$(echo "$time_lines" | sed -n 1p | cut -d'=' -f 2 | xargs | cut -d' ' -f 1)
            comp_time=$(echo "$time_lines" | sed -n 2p | cut -d'=' -f 2 | xargs | cut -d' ' -f 1) # xargs trims white space for us
            time_sum=$(bc <<< $comp_time+$time_sum)
            echo $comp_time
        done
        avg_time=$(bc -l <<< "scale=4; $time_sum/5")
        echo "Average: $avg_time"
        line="$line$avg_time,"
    done
    echo $line >> $out_file
done

cat $out_file