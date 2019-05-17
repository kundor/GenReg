#!/usr/bin/gawk -f
@load "ordchr" # only gawk 4.2 or later...
function isg6(s) {
   n = ord(s) - 63
#   n = ord[substr(s,0,1)]
   if (n < 1 || n > 63) return 0
   numc = int((n*n - n + 11)/12)
   return length(s) == numc + 1 && s ~ /^[?-~]+$/
}
BEGIN {
#   for (n=0; n < 64; ++n) {
#     ord[sprintf("%c",n+63)] = n
#   }
   gfile = "whoa.g6"
   while (getline dontcare < gfile >= 0) {
     close(gfile)
     ind += 1
     gfile = "whoa" ind ".g6"
   }
}
/^>Z/ {
  tot += $2
  ham += $7
  notham += $9
  timed += $11
  next
}
/^(K.{11}|L.{13}|M.{16}|N.{18}|O.{20}|P.{23}|Q.{26})$/ || isg6($0) {
  print > gfile
  next
}
{
   print FILENAME ": " $0 > "/dev/stderr"
}
END {
  printf "%'d total; %'d hamiltonian, %'d not, %'d timed out\n", tot, ham, notham, timed
}
