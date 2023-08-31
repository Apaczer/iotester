#!/bin/sh
echo 2.0 > ./debian-binary
tar --owner=0 --group=0 -czvf ./data.tar.gz -C .//data/ .
tar --owner=0 --group=0 -czvf ./control.tar.gz -C ./control/ .
ar r ../iotester.ipk ./control.tar.gz ./data.tar.gz ./debian-binary
