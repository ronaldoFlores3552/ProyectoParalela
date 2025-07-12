# ProyectoParalela

## ParteA del proyecto

### INSTALACIONES

> sudo apt update

> wget https://developer.download.nvidia.com/compute/cuda/repos/ubuntu2004/x86_64/cuda-keyring_1.0-1_all.deb
> sudo dpkg -i cuda-keyring_1.0-1_all.deb
> sudo apt-get update

> wget http://security.ubuntu.com/ubuntu/pool/universe/n/ncurses/libtinfo5_6.3-2ubuntu0.1_amd64.deb
> sudo apt install ./libtinfo5_6.3-2ubuntu0.1_amd64.deb

> sudo apt-get install cuda-toolkit-12-0

> sudo apt install nvidia-cuda-toolkit
> nvcc --version
> nvidia-smi

### Ejecuciones

> g++ -o generator src/test_generator.cpp src/generate_data.cpp -std=++11 -fopenmp

#### **Ejecutar en Secuencial**

> g++ -std=c++11 -O2 -o mainOutput main.cpp marching_cube_serial.cpp

> archivo generado **mainOutput**

> probar con **mainOutput test_sphere_xx.bin**

#### **Ejecutar en Paralelo**

> make
> archivo generado **marchingCubesParallel**
