#!/bin/bash
#
#
# This shell script will build ALi static memory mapping and
# putout mapping into mapping.dtsi file
#

high_mem_addr=0
set_priv_area_buff_size()
{
	((priv_mem_buff_tatall_size=0x2000000))
	priv_main_flag=0
	buff_enable_flag=0
	priv_mem_buff_size=0
	buff_name=0
	buff_size=0
	cala_priv_area_buff_size=0
	
	#calcaute the rest buff size
	while read line
	do
		if [ ${line:0:1} != "#" -a ${line:0:2} != "//" ];then

			#get priv area buff
			priv_main_flag=`echo $line | cut -d ' ' -f 4`
			buff_enable_flag=`echo $line | cut -d ' ' -f 3`
			priv_mem_buff_size=`echo $line | cut -d ' ' -f 2`
			buff_name=`echo $line | cut -d ' ' -f 1`
			
			if [ $priv_main_flag = 0 -a $buff_enable_flag = 1 ];then
				#echo $line
				if [ "$buff_name" != "priv_area_buff" ];then

					let buff_size=${buff_size}+${priv_mem_buff_size}

				fi
			fi
		fi
	done < memory_map.config
	cala_priv_area_buff_size=`expr $priv_mem_buff_tatall_size - $buff_size`
	cala_priv_area_buff_size=`echo "ibase=10;obase=16;${cala_priv_area_buff_size}" | bc`
	#echo $cala_priv_area_buff_size

	#find priv_area_buff and set size
	while read line
	do
		if [ ${line:0:1} != "#" -a ${line:0:2} != "//" ];then

			#get priv area buff
			priv_main_flag=`echo $line | cut -d ' ' -f 4`
			buff_enable_flag=`echo $line | cut -d ' ' -f 3`
			priv_mem_buff_size=`echo $line | cut -d ' ' -f 2`
			buff_name=`echo $line | cut -d ' ' -f 1`
			
				if [ "$buff_name" = "priv_area_buff" ];then
						sed -i "/$buff_name/s/$priv_mem_buff_size/0x$cala_priv_area_buff_size/"  memory_map.config	
				fi
		fi
	done < memory_map.config
}

set_memory_addr()
{
	priv_main_flag=0
	buff_enable_flag=0
	mem_buff_size=0
	buff_name=0
	low_addr_buff_flag=0
	addr_flag=0
	buff_addr=0
	tmp_buff_addr=0
	#find priv_area_buff and set size
	while read line
	do
		if [ ${line:0:1} != "#" -a ${line:0:2} != "//" ];then
				#get flag
				origin_buff_addr=`echo $line | cut -d ' ' -f 7`
				addr_flag=`echo $line | cut -d ' ' -f 6`
				low_addr_buff_flag=`echo $line | cut -d ' ' -f 5`
				priv_main_flag=`echo $line | cut -d ' ' -f 4`
				buff_enable_flag=`echo $line | cut -d ' ' -f 3`
				#((mem_buff_size=`echo $line | cut -d ' ' -f 2`))
				buff_name=`echo $line | cut -d ' ' -f 1`

				if [ $addr_flag = 0  -a  $buff_enable_flag = 1 ];then
					buff_addr=`expr $buff_addr + $mem_buff_size`
					tmp_buff_addr=`echo "ibase=10;obase=16;${buff_addr}" | bc`
					sed -i "/$buff_name/s/$origin_buff_addr/0x$tmp_buff_addr/"  memory_map.config	
					((mem_buff_size=`echo $line | cut -d ' ' -f 2`))
				elif [ $addr_flag = 1 -a $buff_enable_flag = 1 ];then
					((buff_addr=`echo $line | cut -d ' ' -f 7 `))
					((mem_buff_size=`echo $line | cut -d ' ' -f 2`))
				fi
		fi
	done  <  memory_map.config
	#caclute the highest memory addr
	high_mem_addr=`expr $mem_buff_size + $buff_addr`
}
check_memory()
{
	priv_main_flag=0
	buff_enable_flag=0
	mem_buff_size=0
	buff_name=0
	low_addr_buff_flag=0
	addr_flag=0
	buff_addr=0
	tmp_buff_addr=0
	((low_addr_buff=0x8c000000))
	((priv_mem_addr=0x86000000))
	while read line
	do
		if [ ${line:0:1} != "#" -a ${line:0:2} != "//" ];then
				#get flag
				((origin_buff_addr=`echo $line | cut -d ' ' -f 7`))
				addr_flag=`echo $line | cut -d ' ' -f 6`
				low_addr_buff_flag=`echo $line | cut -d ' ' -f 5`
				priv_main_flag=`echo $line | cut -d ' ' -f 4`
				buff_enable_flag=`echo $line | cut -d ' ' -f 3`
				((mem_buff_size=`echo $line | cut -d ' ' -f 2`))
				buff_name=`echo $line | cut -d ' ' -f 1`

				if [ $buff_enable_flag = 1 ];then
					#priv mem
					if [ $priv_main_flag = 0 ];then
						if [ $origin_buff_addr -gt $priv_mem_addr ];then
							echo "Error: in $line"
							exit
						fi
					#low mem
					elif [ $low_addr_buff_flag = 1 ];then
						if [ $origin_buff_addr -gt $low_addr_buff ];then
							echo "Error: in $line"
							exit
						fi
					fi
					
				fi
		fi
	done  <  memory_map.config

}

build_mapping()
{

	
	priv_main_flag=0
	buff_enable_flag=0
	mem_buff_size=0
	buff_name=0
	low_addr_buff_flag=0
	addr_flag=0
	buff_addr=0
	tmp_buff_addr=0
	dram_size=0

	if [ -f mapping.dtsi ];then
			rm -rf mapping.dtsi
	fi
	touch mapping.dtsi
	
	#get dram size in dram_size.config file
	while read line
	do

		if [ ${line:0:1} != "#" -a ${line:0:2} != "//" ];then

				line=`echo $line | cut -d ' ' -f 1`
				#echo $dram_size1

				if [ $line = 256 ];then
					dram_size=256
				elif [ $line = 512 ];then
					dram_size=512
				else
					echo "Error:we do not support this dram size!"
					exit
				fi
				
		fi
	done  <  dram_size.config

	((phy_high_addr_512m=0x98000000))
	((phy_high_addr_256m=0x90000000))
	((phy_addr_offset=0x80000000))

	#echo $high_mem_addr
	#get the highest memory addr in  static memory
	echo  "/{" >> mapping.dtsi
	echo "#address-cells = <1>;"  >> mapping.dtsi
	echo "#size-cells = <1>;"  >> mapping.dtsi
	echo "memory{" >> mapping.dtsi
	echo " device_type = \"memory\";" >> mapping.dtsi
	echo " reg = <" >> mapping.dtsi
	if [ $dram_size = 256 ];then

		phy_addr=`expr $high_mem_addr - $phy_addr_offset`
		phy_size=`expr $phy_high_addr_256m - $high_mem_addr`
		phy_addr=`echo "ibase=10;obase=16;${phy_addr}" | bc`
		phy_size=`echo "ibase=10;obase=16;${phy_size}" | bc`
		echo "    0x00000000 0x4000000 " >> mapping.dtsi
		echo "    0x$phy_addr 0x$phy_size " >> mapping.dtsi

	elif [ $dram_size = 512 ];then

		phy_addr=`expr $high_mem_addr - $phy_addr_offset`
		phy_size=`expr $phy_high_addr_512m - $high_mem_addr`
		phy_addr=`echo "ibase=10;obase=16;${phy_addr}" | bc`
		phy_size=`echo "ibase=10;obase=16;${phy_size}" | bc`
		echo "    0x00000000 0x4000000 " >> mapping.dtsi
		echo "    0x$phy_addr 0x$phy_size " >> mapping.dtsi
		echo "    0x38000000 0x8000000 " >> mapping.dtsi
	fi

	echo "  >;" >> mapping.dtsi
	echo "  };" >> mapping.dtsi
	echo "memory-mapping{" >> mapping.dtsi
	echo "  compatible = \"alitech,memory-mapping\";" >> mapping.dtsi

	while read line
	do
		if [ ${line:0:1} != "#" -a ${line:0:2} != "//" ];then
				#get flag
				origin_buff_addr=`echo $line | cut -d ' ' -f 7`
				addr_flag=`echo $line | cut -d ' ' -f 6`
				low_addr_buff_flag=`echo $line | cut -d ' ' -f 5`
				priv_main_flag=`echo $line | cut -d ' ' -f 4`
				buff_enable_flag=`echo $line | cut -d ' ' -f 3`
				mem_buff_size=`echo $line | cut -d ' ' -f 2`
				buff_name=`echo $line | cut -d ' ' -f 1`
				
				if [ $buff_enable_flag = 1 ];then
					echo "  $buff_name  = <$origin_buff_addr  $mem_buff_size>;  " >> mapping.dtsi
				fi
		fi
	done  <  memory_map.config
	echo "	};" >> mapping.dtsi
	echo "};" >> mapping.dtsi
}
main()
{
	if [ ! -f memory_map.config ]; then
		echo "There is no memory_map.config file in current fold"
		exit
	fi

	#calculate priv_area_buff size
	set_priv_area_buff_size
	
	#set memory addr
	set_memory_addr
	
	#check memory addr
	check_memory
	
	#build mapping.dtsi file
	build_mapping

}
main
