#!/bin/sh

if [ $# -ne 1 ] ; then
	echo "Usage: $0 ao3-tags-url"
	echo "E.g.: $0 \"https://archiveofourown.org/tags/Harry%20Potter%20-%20J*d*%20K*d*%20Rowling\""
	exit 1
fi

curl -s "$1" |
grep -A 2 ">Relationships:<" |
grep '<li>.*<\/li>' |
sed 's/\s*<li><a class="tag" href="\([^"]*\)">\([^<]*\)<\/a><\/li>/\2,https:\/\/archiveofourown.org\1\n/g' |
sed '/&amp;/d' |
sed '/^[^,]*\/[^,]*\//d' > shiplist.csv

count=0
skipped=0
while read -r s
do
	ship=$(echo "$s" | cut -d\, -f1 | sed 's/\//,/g')
	url=$(echo "$s" | cut -d\, -f2)
	sleep 1
	curl -s "$url" | grep 'has been made a synonym of' >/dev/null && { echo skip>&2 ; continue; }
	sleep 1
	nwords=$(curl -s "$url/works" | grep -o "[0-9]* Works in" | cut -d ' ' -f 1)
	if [ "$nwords" -gt 1 ] 2> /dev/null
	then
		$((count+=1))
		echo "$nwords,$ship"
	else
		$((skipped+=1))
		echo skip>&2; continue
	fi
done <shiplist.csv | tee counts.csv

echo "count: $count"
echo "skipped: $skipped"
sort -n counts.csv > sorted.csv
