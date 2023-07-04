# Bruno Santos Fernandes
# Gabriel Gatti da Silva
# Thiago Felippe Neitzke Lahass

PROJ_NAME = acsh

compile:
	@ gcc acsh.c -o $(PROJ_NAME) 

run: clean compile
	@ ./$(PROJ_NAME)

val:
	@ valgrind --leak-check=full ./$(PROJ_NAME)

clean:
	@ rm -rf *.o $(PROJ_NAME)


