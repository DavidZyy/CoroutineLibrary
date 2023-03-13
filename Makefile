#main: main.c mthread.c  ctx_swap.s mthread.h
#	gcc -g main.c mthread.c ctx_swap.s -o main
PC: ProdCons.c mthread.c  ctx_swap.s mthread.h
	gcc -g ProdCons.c mthread.c ctx_swap.s -o PC
