default:

release:
	rm -rf scenetone_mikmod/libmikmod-3.1.11
	rm -rf scenetone_sid/esidplay
	rm -rf scenetone_shine

	rm -f group/ABLD.BAT
	rm -f scenetone_mikmod/group/ABLD.BAT
	rm -f scenetone_sid/group/ABLD.BAT
	rm -f scenetone_wav/group/ABLD.BAT

	rm -f data/*.sid data/*.SID data/*.mod data/*.MOD data/*.s3m data/*.S3M data/*.xm data/*.XM

	rm -f sis/*.sis sis/*.SIS
	rm -f sis/*.sisx sis/*.SISX

	/usr/bin/find -name .svn -exec rm -rf {} \;

	/usr/bin/find -name "*~" -exec rm -f {} \;
