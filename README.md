Costo operacion

Secuencial:
- Compilacion: g++ mainPorMaquinaSecuencial.cpp calculador_costos_maquina.cpp  -o  main
- Ejecucion: ./main
  
MPI:
- Compilacion: mpic++ -o main main.cpp calculador_costos_maquina.cpp
- Ejecucion: mpirun -np f ./main -> f la cantidad de nucleos
