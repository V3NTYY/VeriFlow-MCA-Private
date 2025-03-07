### README ###

**To build this project using CMake, first ensure you have version 3.8 or higher. If you don't have this, refer to the INSTALLING CMAKE section.**

### INSTALLING CMAKE ###
Run the following set of commands to remove any old versions, update the apt-list and install the latest version of CMake:

`sudo apt remove --purge cmake`
`sudo apt update`
`sudo apt install cmake`

Verify the installation of CMake is at least version 3.8 or higher, if it is not, refer to the INSTALLING CMAKE ALTERNATIVE section.

### INSTALLING CMAKE ALTERNATIVE ###
**WARNING: This process may take up to an hour since you will build CMake from the source.**

First, remove any old versions of CMake, use the following command:

`sudo apt remove --purge cmake`

Run the following commands to update the apt-list, install the necessary dependencies, download the latest version of CMake, and build it:

```
sudo apt update
sudo apt install -y build-essential libssl-dev
wget https://github.com/Kitware/CMake/releases/download/v3.25.0/cmake-3.25.0.tar.gz
tar -zxvf cmake-3.25.0.tar.gz
cd cmake-3.25.0
./bootstrap
make
sudo make install
```

Once the commands are finished (may take over an hour to fully complete), verify the installation of CMake is at least version 3.8 or higher with the command:

`cmake --version`

If it is, CMake is installed correctly and you may continue to building the makefile.

### HANDLING CMAKE COMMAND NOT RECOGNIZED ###
If you've ran any command involving CMake after installing it and the linux terminal did not recognize it, run the following commands:

`nano ~/.bashrc`

This will open nano, add the following line to the end of the file:

`export PATH=/usr/local/bin:$PATH`

Write the changes to the file and exit nano, then use the following command to apply the changes:

`source ~/.bashrc`

After this, retry the CMake command, if it doesn't work then I am not sure.

### BUILDING THE MAKEFILE AND RUNNING THE APP ###
To build the makefile, navigate to MCA_VeriFlow within the repository, and use the following command:

`cmake build .`

Once the command has finishes, you will have a makefile. To build the project, use the following command:

`make`

An executable will be generated titled "MCA_VeriFlow" which you you can run with the following command:

`./MCA_VeriFlow`
