abld.bat build gcce urel
cd ../sis
rm -f scenetone_gcce.sis
rm -f scenetone_gcce.sisx
createsis.bat create scenetone_gcce.pkg
mv scenetone_gcce.sis scenetone_gcce.sisx
cd ../group
echo "******** DONE *********"
