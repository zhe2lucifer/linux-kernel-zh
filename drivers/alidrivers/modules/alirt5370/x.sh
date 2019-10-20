#!/bin/sh

for func in `cat ~/multi-define.log`; do
	grep --include "*.[ch]" "$func\ " -r | awk -F ':' '{print $1}' | sort | uniq | xargs -i sed -i "s/$func /${func}_5370 /g" {}
	grep --include "*.[ch]" "$func\ (" -r | awk -F ':' '{print $1}' | sort | uniq | xargs -i sed -i "s/$func (/${func}_5370 (/g" {}
	grep --include "*.[ch]" "$func\ \[" -r | awk -F ':' '{print $1}' | sort | uniq | xargs -i sed -i "s/$func \[/${func}_5370 \[/g" {}
	grep --include "*.[ch]" "$func\[" -r | awk -F ':' '{print $1}' | sort | uniq | xargs -i sed -i "s/$func\[/${func}_5370\[/g" {}
	grep --include "*.[ch]" "$func(" -r | awk -F ':' '{print $1}' | sort | uniq | xargs -i sed -i "s/$func(/${func}_5370(/g" {}
	grep --include "*.[ch]" "$func," -r | awk -F ':' '{print $1}' | sort | uniq | xargs -i sed -i "s/$func,/${func}_5370,/g" {}
	grep --include "*.[ch]" "$func;" -r | awk -F ':' '{print $1}' | sort | uniq | xargs -i sed -i "s/$func;/${func}_5370;/g" {}
	grep --include "*.[ch]" "$func)" -r | awk -F ':' '{print $1}' | sort | uniq | xargs -i sed -i "s/$func)/${func}_5370)/g" {}
done
