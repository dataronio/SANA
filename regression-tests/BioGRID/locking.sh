#!/bin/bash
set -e
CORES=`cores 2>/dev/null || echo 4`
PARALLEL="./parallel -s bash $CORES"  # run in parallel on jenkins
#PARALLEL=bash # if you don't have the parallel program

die() { echo "ERROR: $@" >&2; exit 1
}

PATH="$PATH:`pwd`/scripts"
export PATH

TMPDIR=/tmp/regression-test$$
#trap "/bin/rm -rf $TMPDIR" 0 1 2 3 15
mkdir $TMPDIR

OutputFile="regression-test.result"
ErrorMargin="0.04"
#echo "WARNING: temporarily set error margin way too big while core scores are being tested"
OutputDir="regression-out"
mkdir -p $OutputDir

if [ ! -d "$OutputDir" ]; then
    mkdir $OutputDir
fi

if [ -f $OutpuFile ]; then
    > "$OutputFile"
fi

NL='
'

[ -x ./sana ] || die "can't find ./sana executable"

# ordered by size in number of nodes
nets="`echo RNorvegicus SPombe CElegans MMusculus SCerevisiae AThaliana DMelanogaster HSapiens | newlines`"
export Networks_count=`echo $nets | wc -w`

NUM_FAILS=0
NUM_LOCK=100 # number of nodes to lock
echo -n "Creating random lock files; "
echo -n > $TMPDIR/networks.locking
echo "$nets" |
    awk '{net[NR-1]=$NF}END{for(i=0;i<NR;i++)for(j=i+1;j<NR;j++) printf "%s %s\n",net[i],net[j]}' | while read g1 g2
    do
	echo $g1 $g2 >> $TMPDIR/networks.locking
	paste <(cat networks/$g1.el | newlines | sort -u | randomizeLines | head -$NUM_LOCK) <(cat networks/$g2.el | newlines | sort -u | randomizeLines | head -$NUM_LOCK) > $TMPDIR/$g1-$g2.lock
    done
echo "running lock test."
echo "$nets" | awk '{net[NR-1]=$NF}END{for(i=0;i<NR;i++)for(j=i+1;j<NR;j++) printf "echo Running %s-%s; ./sana -t 1 -s3 1 -g1 %s -g2 %s -lock '$TMPDIR'/%s-%s.lock -o '$TMPDIR'/%s-%s > '$TMPDIR'/%s-%s.progress 2>&1\n",net[i],net[j],net[i],net[j],net[i],net[j],net[i],net[j],net[i],net[j]}' | eval $PARALLEL
echo "Checking SANA Locking Mechanism" | tee -a $OutputFile
cat $TMPDIR/networks.locking | while read Network1 Network2; do
    if [[ `grep -c -x -f "$TMPDIR/$Network1-$Network2.lock" "$TMPDIR/$Network1-$Network2.align"` -ne $NUM_LOCK ]]; then
	touch $TMPDIR/failed
	echo "LOCKING FAILED on $TMPDIR/$Network1-$Network2" >&2
        (( ++NUM_FAILS ))
    fi
done

echo encountered a total of $NUM_FAILS failures
if [ "$NUM_FAILS" -eq 0 ]; then
    /bin/rm -rf $TMPDIR
else
    echo "results in $TMPDIR">&2
fi
