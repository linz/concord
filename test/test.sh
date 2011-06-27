#!/bin/sh

snapdir=../unix/bin
concord=${snapdir}/concord

COORDSYSDEF=cstest/coordsys.def
export COORDSYSDEF

echo "Running concord tests"
echo "Using: $concord"

rm -rf out
mkdir out

${concord} -Z > out/test_version.out
${concord} -L > out/test_crdsys.out

echo Basic conversion with and without coordinate system conversion


${concord} -iNZGD2000,NEH,H -oNZGD2000,NEH,H -N6 in/test1.in out/test1.out > out/test1.txt
${concord} -iNZGD2000,NEH,H -oNZGD2000,NEH,H -N6 in/test1.in  > out/test2.txt


echo XYZ conversion
${concord} -iNZGD2000,NEH,H -oNZGD2000_XYZ -N6 in/test1.in out/test3.out > out/test3.txt 

echo TM projection
${concord} -iNZGD2000,NEH,H -oWELLTM2000,NEH -N6 in/test1.in out/test4.out > out/test4.txt 

echo NZMG projection
${concord} -iNZGD1949,NEH,H -oNZMG,NEH -N6 in/test1.in out/test5.out > out/test5.txt 

echo LCC projection
${concord} -iWGS84,NEH,H -oST57-60_LCC,NEH -N6 in/test1.in out/test6.out > out/test6.txt 


echo PS projection
${concord} -iWGS84,NEH,H -oANT_PS,NEH -N6 in/test1.in out/test7.out > out/test7.txt 

echo Geoid calculation
${concord} -iNZGD2000,NEH,H -oNZGD2000,NEO,H -gnzgtest09 -N6 in/test1.in out/test8.out > out/test8.txt

echo Default geoid - egm96 in this case
${concord} -iNZGD2000,NEH,H -oNZGD2000,NEO,H -N6 in/test1.in out/test8a.out > out/test8a.txt

echo Geoid calculation - invalid geoid
${concord} -iNZGD2000,NEH,H -oNZGD2000,NEO,H -N6 -gNoSuchGeoid in/test1.in out/test8b.out > out/test8b.txt

echo Different output options

${concord} -iNZGD2000,NEH,H -oNZGD2000,NEH,M -N6 in/test1.in out/test9.out > out/test9.txt

${concord} -iNZGD2000,NEH,H -oNZGD2000,NEH,D -N6 in/test1.in out/test10.out > out/test10.txt

${concord} -iNZGD2000,NEH,H -oNZGD2000,ENH -N6 in/test1.in out/test11.out > out/test11.txt 

${concord} -INZGD2000,ENH,D -oNZGD2000,ENO,H -gnzgtest09 -p5 in/test.lln out/test12.out > out/test12.txt

echo Reference frame conversions

echo Bursa wolf
${concord} -iNZGD2000,NEH,H -oWGS84BW,NEH,D -P8 -N6 in/test1.in out/test13.out > out/test13.txt

echo Grid
${concord} -iNZGD2000,NEH,H -oNZGD1949,NEH,D -P8 -N6 in/test1.in  out/test14.out > out/test14.txt

echo Reference frame grid deformation

${concord} -l NZGD2000@2010.0 > out/test15.txt
#echo epoch = 0
${concord} -iNZGD2000,NE,D -oNZTM_D,EN -P4 -N6 in/test15.in out/test15a.out >> out/test15.txt
#echo epoch = 2000.0
${concord} -iNZGD2000,NE,D -oNZTM_D@2000.0,EN -P4 -N6 in/test15.in out/test15b.out >> out/test15.txt
#echo epoch = 2010.0
${concord} -iNZGD2000D,NE,D -oNZTM_D@2010.0,EN -P4 -N6 in/test15.in out/test15c.out >> out/test15.txt

echo Test each coordinate system with official COORDSYSDEF file

unset COORDSYSDEF

for F in `cat crdsyslist.txt`
do
   # echo "Testing ${F}"
   ${concord} -INZGD2000,NEH,H -o${F} -N6 -P6 in/test1.in out/test_${F}.out >> out/crdsys.txt
#> /dev/null
done

echo "======================================================================"
echo "Checking test output"

isok=Y
for C in `ls check`
do
   if ! test -e out/${C}
   then
	echo "Output file ${C} not generated"
        isok=N
   else 
        diff -b -q out/${C} check/${C} > /dev/null
        if test $? -ne 0
        then
	   echo "Output file ${C} does not match check"
           diff -b out/${C} check/${C}
           isok=N
        fi
   fi
done

if test "${isok}" = Y
then
    echo "All tests passed"
else
    echo "Some tests failed as listed above"
fi
