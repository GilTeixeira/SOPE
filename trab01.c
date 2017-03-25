#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include <wait.h>

#define NUMMAXARGS 10
#define NUMMAXARGSTRING 64

void sigint_handler(int signo)
{
 char str1[10];
 do {
	 printf("Do you wat to leave? (y|n)\n");
	 scanf("%s", str1);
 } while(strcmp(str1, "y") != 0 && strcmp(str1, "n") != 0);
 
 if(strcmp(str1, "y") == 0)
 	exit(6);
}

int main(int argc, char *argv[])
{
 DIR *dirp;		//stream do diretorio (like fd)
 struct dirent *direntp; //estrutura de informações acerca de um ficheiro (tipo o link)
 struct stat stat_buf; //estrutura com mais informações acerca de um ficheiro (tipo os dados)
 pid_t pid;
 
 //parao tratamento de sinais: ^C
 struct sigaction action;
 action.sa_handler = sigint_handler;
 sigemptyset(&action.sa_mask);
 action.sa_flags = 0;
 if (sigaction(SIGINT,&action,NULL) < 0) {
	 fprintf(stderr,"Unable to install SIGINT handler\n");
	 exit(1);
 }
 
 //variáveis que são postas pelo utilizador
 int setname = 0, setype = 0, setperm = 0, setprint = 0, setdelete = 0, setexec = 0;
 //variaveis cuja existencia de pende das variáveis anteriores (só existe namechar se setname for == 1)
 char name[64], permissions[32], permissionschar[32];
 int type = -1;//type: f-0, d-1, l-2
 char ** execargs = (char**) malloc(10 * sizeof(char*));
 int i =0, indexofArg=-1;
 for(i=0; i < NUMMAXARGS; i++){
    execargs[i] = (char*) malloc(NUMMAXARGSTRING * sizeof(char));
 }
 
 if (argc < 2){
	 fprintf( stderr, "Usage: %s dir_name [-name file_name] [-type f|d|l] [-perm permissions] [-print] (-delete | (-exec comand;))?\n", argv[0]);
	 exit(1);
 }
 //verificacao dos argumentos
 i = 2;
 while(i < argc) {
 	if (strcmp(argv[i], "-print") == 0){
 		setprint = 1;
 	} else if (strcmp(argv[i], "-delete") == 0){
 		setdelete = 1;
 	} else if (strcmp(argv[i], "-name") == 0){
 		setname = 1;
 		i++;
 		strcpy(name, argv[i]);
 	}else if (strcmp(argv[i], "-type") == 0){
 		setype = 1;
 		i++;
 		if (strcmp("f", argv[i]) == 0){
 			type = 0;
 		} else if (strcmp("d", argv[i]) == 0){
 			type = 1;
 		} else if (strcmp("l", argv[i]) == 0){
 			type = 2;
 		} else{
 			printf("type %s não foi reconhecido\n", argv[i]);
 			fprintf( stderr, "Usage: %s dir_name [-name file_name] [-type f|d|l] [-perm permissions] [-print] [-delete] [-exec comand;]\n", argv[0]);
			exit(1);
 		}
 	}else if (strcmp(argv[i], "-perm") == 0){
 		setperm = 1;
 		i++;
 		strcpy(permissions, argv[i]);
 	}else if (strcmp(argv[i], "-exec") == 0 && setdelete == 0){
 		setexec = 1;
 		i++;
 		int acm = 0;
 		while(argv[i] != NULL){
 			strcpy(execargs[acm], argv[i]);
 			if(strcmp(argv[i], "{}") == 0){
 				indexofArg = acm;
 			}
 			acm++;
 			i++;
 		}
 		execargs[acm] = NULL;
 		i++;
 	}else if (strcmp(argv[i], "-exec") == 0 && setdelete == 1){
 		printf("ou delete ou exec\n");
 		exit(3);
 	} else {
 		printf("%s não foi reconhecido\n", argv[i]);
 		fprintf( stderr, "Usage: %s dir_name [-name file_name] [-type f|d|l] [-perm permissions] [-print] [-delete] [-exec comand;]\n", argv[0]);
		exit(1);
 	}
 	i++;
 }
 
 //abre o stream no diretorio
 if ((dirp = opendir( argv[1])) == NULL)
 {
 	perror(argv[1]);
 	exit(2);
 }
 
 //printf("%-25s : %10s %10s\n", "Name", "premissions", "size");
 
 //percorre o diretorio
 while ((direntp = readdir( dirp)) != NULL) //abre um determinado link do diretorio
 {
 char newdir[128];
 strcpy(newdir, argv[1]);
 strcat(newdir, "/");
 strcat(newdir, direntp->d_name);
 
 if(stat(newdir, &stat_buf) == -1){ //abre o estrutura de dados do link
 	perror("errou");
 	printf("%s\n",argv[1]);
 	exit(1);
 }
 
 //vai para o subdiretorrio
 if(S_ISDIR(stat_buf.st_mode) && strcmp(direntp->d_name,".") != 0 && strcmp(direntp->d_name,"..") != 0){
 	
	char ** rArray = (char**) malloc(argc * sizeof(char*));
	for(i=0; i < argc; i++){
		if(i != 1)
			rArray[i] = (char*) malloc(strlen(argv[i]) * sizeof(char));
		else
			rArray[i] = (char*) malloc(NUMMAXARGSTRING * sizeof(char));
	}
	
	for(i=0; i < argc; i++){
		if(i != 1)
			strcpy(rArray[i],argv[i]);
		else
			strcpy(rArray[1], newdir);	
	}
	rArray[argc] = NULL;
	
	pid=fork();
 	if (pid == 0){//filho
 		char programe[NUMMAXARGSTRING];
 		char absolutepath[NUMMAXARGSTRING];
 		getcwd(absolutepath, NUMMAXARGSTRING);
 		strcpy(programe, absolutepath);
 		strcat(programe, "/");
 		strcat(programe, "trab01");
 		execv(programe, rArray);
 		perror("erro");
 		exit(4);
 	}
 	wait(NULL);
 }
 
 //ve o nome
 if(setname == 1 && strcmp(direntp->d_name, name) != 0){
 	continue;
 }
 
 //ve o tipo
 if(setype == 1 && !((type == 0 && S_ISREG(stat_buf.st_mode)) ||
 		     (type == 1 && S_ISDIR(stat_buf.st_mode)) ||
 		     (type == 2 && S_ISLNK(stat_buf.st_mode)))){
 	continue;
 }
 
 //ve as premicoes	
 strcpy(permissionschar, (S_ISDIR(stat_buf.st_mode)) ? "d" : "-");
 strcat(permissionschar, (stat_buf.st_mode & S_IRUSR) ? "r" : "-");
 strcat(permissionschar, (stat_buf.st_mode & S_IWUSR) ? "w" : "-");
 strcat(permissionschar, (stat_buf.st_mode & S_IXUSR) ? "x" : "-");
 strcat(permissionschar, (stat_buf.st_mode & S_IRGRP) ? "r" : "-");
 strcat(permissionschar, (stat_buf.st_mode & S_IWGRP) ? "w" : "-");
 strcat(permissionschar, (stat_buf.st_mode & S_IXGRP) ? "x" : "-");
 strcat(permissionschar, (stat_buf.st_mode & S_IROTH) ? "r" : "-");
 strcat(permissionschar, (stat_buf.st_mode & S_IWOTH) ? "w" : "-");
 strcat(permissionschar, (stat_buf.st_mode & S_IXOTH) ? "x" : "-");
 if(setperm == 1 && strcmp(permissionschar, permissions) != 0){
 	continue;
 }
 
 //faz o print
 if(setprint){
 	printf("%-35s : %10s %10lu\n", newdir, permissionschar, stat_buf.st_size);
 }
 
 //faz o delete
 if(setdelete){
 	if(remove(newdir) == -1)
 		printf("did not remove %s\n", newdir);
 }
 
 //faz o exec
 if(setexec){
 	pid=fork();
 	if (pid == 0){ //filho
 		if(indexofArg != -1)
 			strcpy(execargs[indexofArg], newdir);
 		execvp(execargs[0], execargs);
 		perror("erro");
 		exit(4);
 	}
 	wait(NULL);
 }
 }
 closedir(dirp);
 exit(0);
}
