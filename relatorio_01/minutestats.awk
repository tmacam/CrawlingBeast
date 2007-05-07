BEGIN {start_ts=0}
{
   if(start_ts == 0){start_ts = $1}

   ts = ($1 - start_ts)/60;
   sum_s += $3;
   sum_c += $7;
   sum_d += $11;


   if(NR % 6== 0 ) {
	print ts, sum_s, sum_c, sum_d;
	sum_s = 0;
	sum_c = 0;
	sum_d = 0;
   }
}
