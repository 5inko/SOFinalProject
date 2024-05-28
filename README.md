This is a project that consists in a driver that was created to make a buzzer sound using a raspberry pi.
We created a short guide to help you know how to start the project and get the buzzer going!

To run this project you need to follow the following steps:

**[1]** In the project path, run:
make

**[2]** Then run the following line:
sudo insmod Imperial_March_Driver.ko

**[3]** And finally:
echo 1 > /dev/etx_device
