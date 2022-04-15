##  Build mikmod
cd scenetone_mikmod/group
bldmake.bat bldfiles
./abld.bat reallyclean
./abld.bat build gcce urel
cd ../..

## Build sid
cd scenetone_sid/group
bldmake.bat bldfiles
./abld.bat reallyclean
./abld.bat build gcce urel
cd ../..

## Build wav
cd scenetone_wav/group
bldmake.bat bldfiles
./abld.bat reallyclean
./abld.bat build gcce urel
cd ../..

## Build scenetone
cd group
bldmake.bat bldfiles
./abld.bat reallyclean
./abld.bat build gcce urel
cd ..

## Build install file
cd sis
createsis.bat create scenetone_gcce.pkg

