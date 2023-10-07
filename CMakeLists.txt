cmake_minimum_required(VERSION 3.24)
project(laborations C)

set(CMAKE_C_STANDARD 99)

add_executable(laborations_lab1_task1 lab1_task1.c)
target_link_libraries(laborations_lab1_task1)

add_executable(laborations_lab1_task2 lab1_task2.c)
target_link_libraries(laborations_lab1_task2)

add_executable(laborations_lab1_task3 lab1_task3.c)
target_link_libraries(laborations_lab1_task3)

add_executable(laborations_lab2_task1 lab2_task1.c)
target_link_libraries(laborations_lab2_task1 pthread)

add_executable(laborations_lab2_task2 lab2_task2.c)
target_link_libraries(laborations_lab2_task2 pthread)

add_executable(laborations_lab2_task3 lab2_task3.c)
target_link_libraries(laborations_lab2_task3 pthread)

add_executable(laborations_lab2_task4_fcfs lab2_task4_fcfs.c)

add_executable(laborations_lab2_task4_sjf lab2_task4_sjf.c)
