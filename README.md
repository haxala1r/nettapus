# nettapus
A beginner's OS project. Made to learn about OS development. As of now, it does not contain anything of interest or use. You can use the build script to get an image you can run on qemu, as well as real hardware. The "qemu.sh" shell script creates a loopback device and runs this image. The clean script will clean everything. Quite simple, huh?

# Dependencies
This project requires you to have a i686 cross compiler, assembler, archiver and linker (I use gcc, as, ar and ld respectively) to build. Otherwise you can build it yourself as well, of course, by changing the necessary variables in "config.sh" shell script. You also need to have grub installed, since the build script uses "grub-mkrescue" to create the disk image. 



Yeah, this project's still pretty immature as of now. Any contribution is really appreciated.
