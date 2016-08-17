
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#define min(a,b) (a<b?a:b)


struct meta_data {
  // size of the memory allocated by malloc
  size_t size; 		// size of block allocated for USER
  struct meta_data *next;
} ;

struct meta_data* check_best_free(size_t size);
void insert_meta_data(struct meta_data *md);
void connect(struct meta_data * curr);
void remove_meta_data(struct meta_data *ptr);
void copy_data(struct meta_data * final, struct meta_data * initial);
void break_up(struct meta_data *curr, size_t tot_size, size_t data_size);

static struct meta_data *head = NULL;
static struct meta_data *tail = NULL;
static int num_free=0;

/**
 * Allocate space for array in memory
 *
 * Allocates a block of memory for an array of num elements, each of them size
 * bytes long, and initializes all its bits to zero. The effective result is
 * the allocation of an zero-initialized memory block of (num * size) bytes.
 *
 * @param num
 *    Number of elements to be allocated.
 * @param size
 *    Size of elements.
 *
 * @return
 *    A pointer to the memory block allocated by the function.
 *
 *    The type of this pointer is always void*, which can be cast to the
 *    desired type of data pointer in order to be dereferenceable.
 *
 *    If the function failed to allocate the requested block of memory, a
 *    NULL pointer is returned.
 *
 * @see http://www.cplusplus.com/reference/clibrary/cstdlib/calloc/
 */
void *calloc(size_t num, size_t size) {
  void * ret_val = malloc(num*size);
  if(!ret_val) return NULL;
  
  memset(ret_val, 0, size*num);
  return ret_val;
}

/**
 * Allocate memory block
 *
 * Allocates a block of size bytes of memory, returning a pointer to the
 * beginning of the block.  The content of the newly allocated block of
 * memory is not initialized, remaining with indeterminate values.
 *
 * @param size
 *    Size of the memory block, in bytes.
 *
 * @return
 *    On success, a pointer to the memory block allocated by the function.
 *
 *    The type of this pointer is always void*, which can be cast to the
 *    desired type of data pointer in order to be dereferenceable.
 *
 *    If the function failed to allocate the requested block of memory,
 *    a null pointer is returned.
 *
 * @see http://www.cplusplus.com/reference/clibrary/cstdlib/malloc/
 */
void *malloc(size_t size) {
  size_t tot_size = size + sizeof(struct meta_data);
  struct meta_data *ret_val = check_best_free(tot_size);
  if(!ret_val){		// If no viable available memory
	ret_val = sbrk(tot_size);
	if(ret_val == (void*)-1) 		// If sbrk didn't have enough room
		return NULL;
	ret_val->size = size;
	ret_val->next = NULL;
	insert_meta_data(ret_val); 	// insert meta_data into used linked list
	return (void*)ret_val + sizeof(struct meta_data);
  }
  remove_meta_data(ret_val);
  return (void*)ret_val + sizeof(struct meta_data);
}

// runs in O(N) for worst case
struct meta_data* check_best_free(size_t size){
	if(head == NULL) return NULL;
	struct meta_data * best = head;
	struct meta_data * temp = head;
	size_t best_diff = 15000000;

	while(temp){				// parse through free memory and find best fit 
		if((size-sizeof(struct meta_data))>temp->size){} 			// if node isn't big enough 
		
		else{
			size_t diff = temp->size - (size-sizeof(struct meta_data));
			if(diff == 0){
				num_free--;
				return temp;
			}
			else if (diff < best_diff){
				best_diff = diff;
				best = temp;
			}
		}
		temp = temp->next;
	}
	if((!best) ||((best == head) && (best->size < (size-sizeof(struct meta_data))))){ // didn't find a capable size of memory
		//printf("No viable link\n");
		return NULL;
	}
	else {
		if(best_diff > sizeof(struct meta_data))
			break_up(best, best->size, size-sizeof(struct meta_data));	
		num_free--;	
		return best;
	}
}

void break_up(struct meta_data *curr, size_t tot_size, size_t data_size){
	curr->size = data_size;
	struct meta_data * new_node = (void*)curr + data_size + sizeof(struct meta_data);
	new_node->size = tot_size - data_size - sizeof(struct meta_data);
	new_node->next = NULL;
	insert_meta_data(new_node);
	connect(new_node);
}

// insert at beginning of linked list
void insert_meta_data(struct meta_data *md) {
  if(md == NULL) return;

  if(head == NULL){ 		// If linked list is empty
	head = md;
	tail = md;
  }
  else{
	struct meta_data *temp = tail;
	tail = md;
	temp->next = tail;
  }
}



/**
 * Deallocate space in memory
 *
 * A block of memory previously allocated using a call to malloc(),
 * calloc() or realloc() is deallocated, making it available again for
 * further allocations.
 *
 * Notice that this function leaves the value of ptr unchanged, hence
 * it still points to the same (now invalid) location, and not to the
 * null pointer.
 *
 * @param ptr
 *    Pointer to a memory block previously allocated with malloc(),
 *    calloc() or realloc() to be deallocated.  If a null pointer is
 *    passed as argument, no action occurs.
 */
void free(void *ptr) {
  if(!ptr) return;
  num_free ++;
  struct meta_data * curr = (struct meta_data *)(ptr - sizeof(struct meta_data)); 	// meta_data reppin the ptr
  //printf("struct size: %zu\n", sizeof(struct meta_data));
  insert_meta_data(curr);
  connect(curr);
  
}

// connects adjacent meta_datas if both freee
void connect(struct meta_data * curr){
  struct meta_data *temp = head;
  struct meta_data *temp1 = head;

  while(temp){
	if(temp != curr){
		if(((void*)temp + temp->size + sizeof(struct meta_data)) == curr){			// if block is free and before curr
			temp->size += curr->size + sizeof(struct meta_data);
			printf("found free block before, total size is %zu\n", curr->size);
			remove_meta_data(curr);
		}
		if(((void*)curr + curr->size + sizeof(struct meta_data)) == temp){ 		// if block is free and after curr
			curr->size += temp->size + sizeof(struct meta_data);
			printf("found free block after, total size is %zu\n", curr->size);
			temp1 = temp;
			temp = temp->next;
			remove_meta_data(temp1);
			
		}
		else
			temp = temp->next;
	}
	else 
		temp = temp->next;
	
  }

}

// removes a meta_data node from linked list
void remove_meta_data(struct meta_data *ptr) {
  struct meta_data *curr_node = head;
  struct meta_data * prev = NULL;

  if(!ptr){ 			// if ptr is NULL
	//printf("node trying to be deleted is null\n");
	return;
  }

  while(curr_node){
	
	if(curr_node ==  ptr){  // if we found ptr in our linked list
		if(prev){ 			// if ptr isn't the head 
			prev->next = curr_node->next;
			curr_node->next = NULL;
		}
		else if(head){ 				// if ptr is the head and there is a head
			head = head->next;
			curr_node->next = NULL;
		}
		return;
	}
	prev = curr_node;
    curr_node = curr_node->next;
  }
  //printf("node is not in the list\n");	//couldn't find ptr in linked list
}

/**
 * Reallocate memory block
 *
 * The size of the memory block pointed to by the ptr parameter is changed
 * to the size bytes, expanding or reducing the amount of memory available
 * in the block.
 *
 * The function may move the memory block to a new location, in which case
 * the new location is returned. The content of the memory block is preserved
 * up to the lesser of the new and old sizes, even if the block is moved. If
 * the new size is larger, the value of the newly allocated portion is
 * indeterminate.
 *
 * In case that ptr is NULL, the function behaves exactly as malloc, assigning
 * a new block of size bytes and returning a pointer to the beginning of it.
 *
 * In case that the size is 0, the memory previously allocated in ptr is
 * deallocated as if a call to free was made, and a NULL pointer is returned.
 *
 * @param ptr
 *    Pointer to a memory block previously allocated with malloc(), calloc()
 *    or realloc() to be reallocated.
 *
 *    If this is NULL, a new block is allocated and a pointer to it is
 *    returned by the function.
 *
 * @param size
 *    New size for the memory block, in bytes.
 *
 *    If it is 0 and ptr points to an existing block of memory, the memory
 *    block pointed by ptr is deallocated and a NULL pointer is returned.
 *
 * @return
 *    A pointer to the reallocated memory block, which may be either the
 *    same as the ptr argument or a new location.
 *
 *    The type of this pointer is void*, which can be cast to the desired
 *    type of data pointer in order to be dereferenceable.
 *
 *    If the function failed to allocate the requested block of memory,
 *    a NULL pointer is returned, and the memory block pointed to by
 *    argument ptr is left unchanged.
 *
 * @see http://www.cplusplus.com/reference/clibrary/cstdlib/realloc/
 */
void *realloc(void *ptr, size_t size) {
   if(!ptr){			// if ptr is null act like malloc
	return malloc(size); 		
  }

   if(size == 0){
	free(ptr);
	return NULL;
  }  
  
  struct meta_data * curr = ptr - sizeof(struct meta_data);

  if(curr->size >= size){ 	// If current block has enough memory already (break up if too much memory)
	size_t diff = curr->size - size;
	if(diff > sizeof(struct meta_data))
		break_up(curr, curr->size, size);
	return ptr;
  }	
  
  
  if(size > curr->size){ 		// If current block doesn't have enough space
	struct meta_data * ret_val = (struct meta_data*)(malloc(size) - sizeof(struct meta_data));
	copy_data(ret_val, curr);
	free(ptr);
	return (void*)ret_val + sizeof(struct meta_data);
	}
  return NULL;
}

void copy_data(struct meta_data * final, struct meta_data * initial){
	for(size_t x=0; x<min(final->size, initial->size); x++){
		*((char*)final+sizeof(struct meta_data)+x) = *((char*)initial+sizeof(struct meta_data)+x);
	}
}
