#!/bin/bash
#
#
# This shell script will check ALi static memory mapping out and
# give a map about the memory mapping.
#

put_info()
{
		printf "\e[32m $1 \e[0m \n"
}
prinf_middle_info()
{
		printf "\033[1A"
		printf "\033[5C"
		printf "\e[32m MEM_NAME:$1 MEM_SIZE:0x%x MEM_END:0x%x \e[0m \n" $2 $3
}
prinf_middle_err()
{
		printf "\033[1A"
		printf "\033[5C"
		printf "\e[31m MEM_NAME:$1 MEM_SIZE:0x%x MEM_END:0x%x \e[0m \n" $2 $3
}

prinf_right_info()
{
		printf "\033[1A"
		printf "\033[68C"
		#1->red 2->green 3->yellow
		if [ $2 -eq  1 ];then
			printf "\e[31m<---MEM_BUF_START:0x%x \e[0m \n" $1
		elif [ $2 -eq  2 ];then
			printf "\e[32m<---MEM_BUF_START:0x%x \e[0m \n" $1
		elif [ $2 -eq  3 ];then
			printf "\e[31m<---LAST_MEM_BUF_END:0x%x \e[0m \n" $1
		elif [ $2 -eq  4 ];then
			printf "\e[32m<---LAST_MEM_BUF_END:0x%x \e[0m \n" $1
		fi
}

main()
{
	if [ $# -lt 1 ]; then
  	put_erro "Need to input the mapping file!!"
  	exit 1
	fi
	i=0
	while read line
	do
		if echo $line | grep -q "=" ;then
			if ! echo $line | grep -q "#" ;then
				if echo $line | grep -q "<" ;then
					if echo $line | grep -q ">" ;then
						mem_name[i]=`echo $line | awk -F "=" '{print $1}'`
						((mem_start[i]=`echo $line | awk -F "=" '{print $2}' | awk -F " " '{print $1}' | awk -F "<" '{print $2}' `))
						((mem_size[i]=`echo $line | awk -F "=" '{print $2}' | awk -F " " '{print $2}' | awk -F ">" '{print $1}' `))
						let i++
					fi
				fi
			fi
		fi
	done  < $1

	# set mem_start_addr
	for((k=0;k<$i;k++));
	do
		for((j=0;j< $i - 1;j++));
		do  
				if [ ${mem_start[j]} -lt ${mem_start[j+1]} ]; then 	
					mem_start_tmp=${mem_start[j+1]}
					mem_size_tmp=${mem_size[j+1]}
					mem_name_tmp=${mem_name[j+1]}
						
					mem_start[j+1]=${mem_start[j]}
					mem_size[j+1]=${mem_size[j]}
					mem_name[j+1]=${mem_name[j]}
					
					mem_start[j]=$mem_start_tmp 
					mem_size[j]=$mem_size_tmp
					mem_name[j]=$mem_name_tmp

				fi 
		done;
	done;
	#static address for ALi chip of memory mapping
	((see_low_addr=0xA4000000))
	((see_high_addr=0xA6000000))
	((main_low_addr=0xA2E0E800))
	see_start=0
	put_info "|*****************************************************************|"
	put_info "|*****************************************************************|"

  # a for flow will draw the following picuture
  #|-----------------------------------------------------------------|<--MEM_START:XXXXXX
	#|                                                                 |
	#|             	MEM_NAME:xxx MEM_SIZE:xxx 	                       |
	#|                                                                 |
	

	#start check address from small to large
	for((j=$i-1;j>=0;j--))
	do  

			let mem_end=${mem_start[j]}+${mem_size[j]}
		
			#set see memory start flag
			if [ ${mem_start[j]} -ge $see_low_addr -a $see_start -eq 0 ] ; then
				see_start=1
				put_info "|                                                                 |"
				put_info "|*****************************************************************|"
				printf "\033[1A"
				printf "\033[68C"
				printf "\e[33m<---SEE_MEM_BUF_START  \e[0m \n"
			fi
			#set see memory end flag
			if [ ${mem_start[j]} -ge $see_high_addr -a $see_start -eq 1 ] ; then
				see_start=2
				put_info "|                                                                 |"
				put_info "|*****************************************************************|"
				printf "\033[1A"
				printf "\033[68C"
				printf "\e[33m<---SEE_MEM_BUF_END \e[0m \n"
			fi
	
			if [ $j -eq 0 ]; then
			#last memory mapping
							put_info "|-----------------------------------------------------------------|"
							prinf_right_info ${mem_start[j]}  2
							put_info "|                                                                 |"
							put_info "|                                                                 |"
							prinf_middle_info ${mem_name[j]} ${mem_size[j]} ${mem_end}
							put_info "|                                                                 |"
							put_info "|-----------------------------------------------------------------|"
							prinf_right_info ${mem_end}  4 
			else
				if [ ${mem_end}  -gt ${mem_start[j - 1]} ]; then
				#overlap memory mapping
							put_info "|-----------------------------------------------------------------|"
							prinf_right_info ${mem_start[j]} 1
							put_info "|                                                                 |"
							put_info "|                                                                 |"
							prinf_middle_err ${mem_name[j]} ${mem_size[j]} ${mem_end}
							put_info "|                                                                 |"
							
				elif [ ${mem_end}  -eq ${mem_start[j - 1]} ] ; then
				#perfect memory mapping
							put_info "|-----------------------------------------------------------------|"
								prinf_right_info ${mem_start[j]}  2
								put_info "|                                                                 |"
								put_info "|                                                                 |"
								prinf_middle_info ${mem_name[j]} ${mem_size[j]} ${mem_end}
								put_info "|                                                                 |"
				else	
				#has useless space!!!
					
								put_info "|-----------------------------------------------------------------|"
								prinf_right_info ${mem_start[j]} 2
								put_info "|                                                                 |"
								put_info "|                                                                 |"
								prinf_middle_info ${mem_name[j]} ${mem_size[j]} ${mem_end}
								put_info "|                                                                 |"
								put_info "|-----------------------------------------------------------------|"
								let last_mem_addr_end=${mem_start[j]}+${mem_size[j]}
								prinf_right_info ${last_mem_addr_end} 4

								put_info "|                                                                 |"	
								put_info "|                                                                 |"
								printf "\033[1A"
								printf "\033[15C"
								printf "\e[31m useless memory!!! \e[0m \n"
								put_info "|                                                                 |"
				fi
			fi
	done;
	put_info "|*****************************************************************|"
	put_info "|*****************************************************************|"
}

main $1