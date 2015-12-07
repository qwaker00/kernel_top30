pushd ..
make
popd

sudo rmmod top30.ko
sudo insmod ../top30.ko
sudo ./mt_test
