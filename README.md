# Simple-character-device-driver
A simplified version of character device driver used to read, write, seek and change direction.

Support a variable number of devices that can be set at load time (de-fault will be 3). 
The device nodes are named /dev/mycdrv0, /dev/mycdrv1, ..., /dev/mycdrv(N-1), where N is the num-ber of devices. 
The device driver creates the device nodes.

An entry function is provided that would be accessed via lseek() function. 
That entry function updates the ﬁle position pointer based on the oﬀset requested. 
Check is implemented for bound checks and out of bound requests and is set to the closest boundary.

The entry function is accessed via ioctl() function. It lets the user application change the direction of 
data access: regular (default) and reverse. Regular direction means when data needs to be read or written to starting
at an oﬀset o for count number of bytes, the device area, i.e., ramdisk, that is used is 
[ramdisk + o, ramdisk + o + count] (assuming these bounds are legal) so that the ﬁrst byte in the user buﬀer,
e.g., buffer[0], corresponds to *(ramdisk + o), buffer[1] corresponds to *(ramdisk + o + 1), etc. 
When reverse direction is set, however, the device area that is used is ramdisk + o -count, ramdisk + o]
(assuming these bounds are legal) and buffer[0] corresponds to *(ramdisk + o), buffer[1] corresponds to *(ramdisk +o - 1), etc.
Symbol ASP CHGACCDIR is assigned for the command to change the direction of access and the parameter should be 0 for regular
and 1 for reverse. The function sets the access mode to requested mode and return the previous access mode. 

The read and write functions support both regular and reverse access modes and update the ﬁle position pointers accurately.

Each device can be opened concurrently and therefore can be accessed for read, write, lseek, and ioctl concurrently. 
Appropriate synchronization is provided to prevent race con-ditions.

Lastly, all the resources (including the ramdisk, the device structures, and device nodes)
are recycled/freed at unloading time.
