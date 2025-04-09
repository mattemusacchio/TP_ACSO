#include "ej1.h"

string_proc_list* string_proc_list_create(void) {
	string_proc_list* list = malloc(sizeof(string_proc_list));
	if (list == NULL) {
		return NULL;
	}
	list->first = NULL;
	list->last = NULL;
	return list;
}

string_proc_node* string_proc_node_create(uint8_t type, char* hash) {
	string_proc_node* node = malloc(sizeof(string_proc_node));
	if (node == NULL) {
		return NULL;
	}
	node->next = NULL;
	node->previous = NULL;
	node->type = type;
	node->hash = hash;  // No copiamos el hash, solo apuntamos al mismo
	return node;
}

void string_proc_list_add_node(string_proc_list* list, uint8_t type, char* hash) {
	string_proc_node* new_node = string_proc_node_create(type, hash);
	if (new_node == NULL) {
		return;
	}

	if (list->first == NULL) {
		// Lista vacía
		list->first = new_node;
		list->last = new_node;
	} else {
		// Lista no vacía
		new_node->previous = list->last;
		list->last->next = new_node;
		list->last = new_node;
	}
}

char* string_proc_list_concat(string_proc_list* list, uint8_t type, char* hash) {
	if (list == NULL || hash == NULL) {
		return NULL;
	}

	// Calcular longitud del hash inicial
	size_t hash_len = strlen(hash);
	
	// Reservar memoria para el resultado inicial
	char* result = malloc(hash_len + 1);
	if (result == NULL) {
		return NULL;
	}
	
	// Copiar el hash inicial
	strcpy(result, hash);

	string_proc_node* current = list->first;
	while (current != NULL) {
		if (current->type == type) {
			char* temp = str_concat(result, current->hash);
			free(result);
			if (temp == NULL) {
				return NULL;
			}
			result = temp;
		}
		current = current->next;
	}

	return result;
}


/** AUX FUNCTIONS **/

void string_proc_list_destroy(string_proc_list* list){

	/* borro los nodos: */
	string_proc_node* current_node	= list->first;
	string_proc_node* next_node		= NULL;
	while(current_node != NULL){
		next_node = current_node->next;
		string_proc_node_destroy(current_node);
		current_node	= next_node;
	}
	/*borro la lista:*/
	list->first = NULL;
	list->last  = NULL;
	free(list);
}
void string_proc_node_destroy(string_proc_node* node){
	node->next      = NULL;
	node->previous	= NULL;
	node->hash		= NULL;
	node->type      = 0;			
	free(node);
}


char* str_concat(char* a, char* b) {
	int len1 = strlen(a);
    int len2 = strlen(b);
	int totalLength = len1 + len2;
    char *result = (char *)malloc(totalLength + 1); 
    strcpy(result, a);
    strcat(result, b);
    return result;  
}

void string_proc_list_print(string_proc_list* list, FILE* file){
        uint32_t length = 0;
        string_proc_node* current_node  = list->first;
        while(current_node != NULL){
                length++;
                current_node = current_node->next;
        }
        fprintf( file, "List length: %d\n", length );
		current_node    = list->first;
        while(current_node != NULL){
                fprintf(file, "\tnode hash: %s | type: %d\n", current_node->hash, current_node->type);
                current_node = current_node->next;
        }
}

