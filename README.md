# I/O tester
Input/output tester for the MiyooCFW

## Compiling instructions
1. Set up your environment with latest SDK preferably uClibc (in distro which use glibc 2.34 version or above e.g. 22.04 Ubuntu): 
```
cd
git clone https://github.com/MiyooCFW/buildroot
cd buildroot
make miyoo_uclibc_defconfig
sudo make sdk
cd output/images
gzip -d arm-miyoo-linux-uclibcgnueabi_sdk-buildroot.tar.gz
tar xvf arm-miyoo-linux-uclibcgnueabi_sdk-buildroot.tar
mv arm-miyoo-linux-uclibcgnueabi_sdk-buildroot miyoo
sudo cp -a miyoo /opt/
```
2. Copy this repo & compile default version (pocketgo)
``` 
git clone https://github.com/Apaczer/iotester
cd iotester
make -f Makefile.miyoo clean
make -f Makefile.miyoo
```
or build distribution package:
```
make -f Makefile.miyoo gm2xpkg-ipk
```

For XYC version add ``DEVICE=XYC`` to passed variables or BITTBOY before compiling.

## FAQ
Q) _What version should I use?_  
A:  For XYC Q8 and SUP M3 it is recommended to use ``xyc`` binary, for BITTBOY the ``bittboy`` and ``default`` for everything else as generic console.
