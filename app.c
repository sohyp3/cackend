#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "unistd.h"
#include "sys/socket.h"
#include "netinet/in.h"


typedef struct{
	char *path;
	char *method;
	char *(*responseFunc)(const char *method);
} PathResponseMap;




char *generateHttpHeader(int code, char *type){
	char *header_code_200  = "HTTP/1.1 200 OK\n";
	char *header_code_404  = "HTTP/1.1 404 Not Found\n";
	char *header_code_405 = "HTTP/1.1 405 Method Not Allowed\n";

	char *type_html = "Content-Type: text/html\n\n";

	char *status_line;	

	if (code == 200){
		status_line = header_code_200;
	}
	else if (code == 404){
		status_line = header_code_404;
	}
	else if (code == 405){

		status_line = header_code_405;
	}
	else{
		perror("invalid code given");
		exit(1);
	}	
	char *header = malloc(strlen(status_line)+strlen(type_html)+1);
	
	if (header == NULL){
		perror("Error allocting memory for header");
		exit(1);
	}


	strcpy(header, status_line);
	strcat(header, type_html);
	


	return header;
}


char *readHtmlFile(const char *file_path){
	char *base_dir = "html/";
	char *full_path = malloc(strlen(base_dir)+strlen(file_path)+1);
	
	strcpy(full_path, base_dir );
	strcat(full_path, file_path);


	FILE *file = fopen(full_path, "r");

	free(full_path);


	if (file == NULL){
		perror("Error opening the HTML file");
		exit(1);
	}

	fseek(file, 0, SEEK_END);
	long file_size = ftell(file);

	fseek(file, 0, SEEK_SET);

	char *html_content = malloc(file_size+1);

	if (html_content){
		fread(html_content, 1, file_size,file); 
		html_content[file_size] = '\0';
	}
	else{
		printf("FILE EMPTY!!");
	}

	fclose(file);
	return html_content;

}



char *fullHeader(const char *file_path, int code){
	char *header = generateHttpHeader(code, "text/html");
	
	if (header == NULL){
		return NULL;
	}
	char *html_content = readHtmlFile(file_path);

	if (html_content == NULL){
		free(header);
		return NULL;
	}

	char *response = malloc(strlen(header)+strlen(html_content)+1);

	if (response == NULL){
		free(header);
		free(response);
		return NULL;
	}

	strcpy(response, header);
	strcat(response, html_content);

	free(header);
	free(html_content);

	return response;
}

char *homePage(const char *method){
	if (strcmp(method, "GET") == 0){
		return fullHeader("home.html",200);
	}
	else {
		return fullHeader("404.html", 404);
	}
}


char *ligmaPage(const char *method){
	return fullHeader("ligma.html",200);
}


PathResponseMap mappings[] = {
	{"/","*",homePage},
	{"/ligma","GET",ligmaPage},
	
	{NULL,NULL,NULL}
};


int main(){
	int listen_sock = socket(AF_INET, SOCK_STREAM, 0);

	if (listen_sock < 0){
		perror("Error Creating Socket!");
		exit(1);
	}


	int port_no = 6969;
	struct sockaddr_in server_addr;

	memset(&server_addr, 0, sizeof(server_addr));

	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(port_no);
	server_addr.sin_family =AF_INET;

	if ( bind(listen_sock, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0 ){
		perror("Error on binding");
		exit(1);
	}

	listen(listen_sock, 5);

	struct sockaddr_in client_addr;
	socklen_t client_addr_len = sizeof(client_addr);

	while (1){
		int connect_sock = accept(listen_sock, (struct sockaddr *) &client_addr, &client_addr_len);
		if (connect_sock < 0){
			perror("Error on accept");
			continue;
		}

		char buffer[1024];
		memset(buffer,0, 1023);
		read(connect_sock,buffer,1023);

		char method[10], path[50], protocol[10];

		sscanf(buffer, "%s %s %s", method,path,protocol);

		printf("%s %s %s", method,path,protocol);

		char *response = NULL;

		for (int i=0; mappings[i].path != NULL;++i){
			if (strcmp(path, mappings[i].path ) == 0){
				if (strcmp(mappings[i].method, method) == 0 || strcmp(mappings[i].method, "*") == 0 ){
					response = mappings[i].responseFunc(method);
					break;
				}
				else{
					response = fullHeader("405.html", 405);
					break;
				}
			}
		}

		if (response == NULL){
			response = fullHeader("404.html",404);
		}

		write(connect_sock, response, strlen(response));

		free(response);
		close(connect_sock);

	}
	close(listen_sock);

	return 0;
}


