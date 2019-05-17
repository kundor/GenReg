#!/bin/awk -f
function fivenums(nums) {
    asort(nums)
    ln = length(nums) - 1
    print nums[1], nums[int(ln/4+1)], nums[int(ln/2+1)], nums[int(3*ln/4+1)], nums[ln+1]
}
function meanstd(nums) {
    for (i = 1; i <= length(nums); ++i) {
        delta = nums[i] - M
        M += delta/i
        C += delta*(nums[i] - M)
    }
    print "mean", M, "std", sqrt(C/(length(nums) - 1));
}
BEGIN {
    split("", times); # create empty arrays
    split("", numg);
}
/^>Z/ {
    numg[length(numg)+1] = $2 
    times[length(times)+1] = $14
}
END {
    fivenums(times)
    meanstd(times)
    fivenums(numg)
    meanstd(numg)
}
