filename=currsetting.c
resultname=result
fullresult=fullresult.c
touch ${filename}

echo "--------------------------settings.h-----------------------------" > ${filename}
cat include/settings.h >> ${filename}
echo "----------------------------main.c-------------------------------" >> ${filename}
cat interface/main.c >> ${filename}

./simulator | tee ${resultname}
echo "simulator end!"

touch ${fullresult}
cat ${filename} > ${fullresult}
echo "-----------------------simaultor result-------------------------------" >> ${fullresult}
cat ${resultname} >> ${fullresult}
rm ${filename}
rm ${resultname}
