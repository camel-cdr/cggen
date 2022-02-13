#/usr/bin/env bash

if [ $# -ne 1 ] ; then
	echo "Usage: $0 ao3-tags-url"
	echo "E.g.: $0 \"https://archiveofourown.org/tags/Harry%20Potter%20-%20J*d*%20K*d*%20Rowling\""
	exit 1
fi

curl "$1" |
grep -A 2 ">Relationships:<" |
grep '<li>.*<\/li>' |
sed 's/\s*<li><a class="tag" href="\([^"]*\)">\([^<]*\)<\/a><\/li>/\2,https:\/\/www.archiveofourown.org\1\n/g' |
sed '/&amp;/d'  |
sed '/^[^,]*\/[^,]*\//d' > shiplist.csv

lines=$(cat shiplist.csv)
IFS=$'\n'
for s in $lines
do
	ship=`echo "$s" | cut -d\, -f1 | sed 's/\//,/g'`
	url=`echo "$s" | cut -d\, -f2`
	sleep 0.1
	curl "$url" | grep 'has been made a synonym of' >/dev/null && continue
	sleep 0.1
	printf "%s,%s\n" `curl "$url/works" | grep "[0-9]* Works in" | sed 's/^.*\([0-9][0-9]*\) Works in.*$/\1/'` "$ship"
done #| tee counts.csv

sort -n -t ',' -k 3 counts.csv > sorted.csv
