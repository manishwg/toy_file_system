#include "api.h"



int filewrite()
{
    char c;
    FILE * fp;
    int needed_block,file_size,buffcount=0;
	unsigned int tmp_offset;

	int file_inode_index;
	inode *file_inode;

    data_block *buff;
    buff = (data_block *) malloc(sizeof(data_block));

	fp = fopen ("bigfile.txt", "r");
    fseek(fp, 0, SEEK_END);
    file_size = ftell(fp);
    needed_block = ceil(file_size*1.0/(BS));

    if(sb_object->no_free_inodes < needed_block){
        printf("E: not enough memory");
        return 0;
    }

    fseek(fp, 0, SEEK_SET);
	// allocate inode
	file_inode_index = sb_object->next_free_inode_index;
	inode_bmap_object->inode_bmap_index[file_inode_index] = 1;
	// fill data inode
	file_inode = (inode *) &inode_table_object->inode_index[file_inode_index];
	file_inode->file_type = 0;
	file_inode->size = 0;

	printf("(in write.c filewrite()): ***file_size %d***\n/",file_size);


    while(c=fgetc(fp)){
		if(buffcount == BS || c==EOF){
			tmp_offset = get_next_offset_to_write(file_inode);
			printf("(in write.c filewrite()): *** db_block_write_offset(%d): %d  size: %d ***\n", buffcount, tmp_offset,file_inode->size);
			my_write(buff,buffcount,tmp_offset);
			set_next_free_block();
			file_inode->size += buffcount;
			buffcount=0;
			write_fs();
		}
		if(c!=EOF){
			buff->data[buffcount]=c;
			//printf("%d\n",buffcount);
			buffcount++;
		}else{
			break;
		}
	}
	
    free(buff);
	set_next_free_inode_index();
    return file_inode_index;
}

unsigned int get_next_offset_to_write(inode *file_inode)
{
	int file_size = file_inode->size;
	int remain_block;
	int si_index,si_remblock,tmp_si_offset=0;
	unsigned int new_si_off;
	si_block * si_block_obj;
	si_block_obj = (si_block *) malloc(sizeof(si_block));

	if(file_size < sizeof(data_block)){
		file_inode->direct_block_offset =  sb_object->next_free_data_block_offset;
		return file_inode->direct_block_offset;

	}else if(file_size == sizeof(data_block)){


		file_inode->single_indirect_block_offset = sb_object->next_free_data_block_offset;
 		set_next_free_block();
		si_block_obj->db_index[0] = sb_object->next_free_data_block_offset;
		my_write(si_block_obj,sizeof(si_block),file_inode->single_indirect_block_offset);

		return sb_object->next_free_data_block_offset;
	}else{

		file_size -= sizeof(data_block);
		remain_block = ceil((file_size*1.0)/sizeof(data_block));
		si_index = remain_block /((sizeof(data_block)/sizeof(int *))-1);
		si_remblock = remain_block %((sizeof(data_block)/sizeof(int *))-1);


		if(!si_remblock)
			si_index--;
		tmp_si_offset = file_inode->single_indirect_block_offset;
		my_read(si_block_obj,sizeof(si_block),file_inode->single_indirect_block_offset);
		while(si_index--){
			tmp_si_offset = si_block_obj->db_index[BS/sizeof(int *)-1] ;
			my_read(si_block_obj,sizeof(si_block),tmp_si_offset);
		}
		if(!si_remblock){
			si_block_obj->db_index[BS/sizeof(int *)-1] = sb_object->next_free_data_block_offset;
			my_write(si_block_obj,sizeof(si_block),tmp_si_offset);
			tmp_si_offset = sb_object->next_free_data_block_offset;
			set_next_free_block();
			si_block_obj->db_index[0] = sb_object->next_free_data_block_offset;
			my_write(si_block_obj,sizeof(si_block),tmp_si_offset);

			return sb_object->next_free_data_block_offset;

		}else{
			//printf("**************** rem block:%d  si_index: %d si_remblock: %d \n",remain_block,si_index ,si_remblock);
			my_read(si_block_obj,sizeof(si_block),tmp_si_offset);
			si_block_obj->db_index[si_remblock]=sb_object->next_free_data_block_offset;
			my_write(si_block_obj,sizeof(si_block),tmp_si_offset);
			return sb_object->next_free_data_block_offset;
		}
	}
	free(si_block_obj);

}



int write_fs(){
	my_write(sb_object,sizeof(sb),0);
	my_write(db_bmap_object,sizeof(db_bmap),sb_object->data_block_bmap_offset);
	my_write(inode_bmap_object,sizeof(*inode_bmap_object),sb_object->inode_bmap_offset);
	my_write(inode_table_object,sizeof(*inode_table_object),sb_object->inode_table_offset);
	return 0;
}

int set_next_free_block(){
	int i,tmp_offset;
	sb_object->no_free_blocks--;
	tmp_offset = (sb_object->next_free_data_block_offset - sb_object->data_block_offset)/sizeof(data_block);


	db_bmap_object->db_bmap_index[tmp_offset]=1;

	for(i=0;i<sizeof(db_bmap);i++){
		if(!db_bmap_object->db_bmap_index[i]){
			sb_object->next_free_data_block_offset=sb_object->data_block_offset+i*sizeof(data_block);
			return 1;
		} 
	}
	return 0;
}

int set_next_free_inode_index(){
 	int i;
	for(i=0;i<(sizeof(inode_bmap));i++)
		if(!inode_bmap_object->inode_bmap_index[i]){
			sb_object->next_free_inode_index=i;
			return 1;
		} 

	return 0;
}
