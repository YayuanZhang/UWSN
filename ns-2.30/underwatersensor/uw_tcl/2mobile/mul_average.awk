BEGIN {
  looptime=10;
  sum3[30]=0;
  sum4[30]=0;
  sum5[30]=0;
  sum6[30]=0;
  sum7[30]=0;
  sum8[30]=0;
  sum9[30]=0;
  sum10[30]=0;
  cnt[30]=0;
}
{ 
for(i=1;i<33;i++)
  { 
	if ($1==0.001*i)
        {   cnt[i]++;
            if(cnt[i]<=looptime)
              {
		sum3[i]+=$3;
                sum4[i]+=$4;
		sum5[i]+=$5;
                sum6[i]+=$6;
 		sum7[i]+=$7;
                sum8[i]+=$8;
		sum9[i]+=$9;
                sum10[i]+=$11;		
              }
            if(cnt[i]==looptime)
		{ 
			printf("%f %f %f %d %d %f %f %f %f %f\n",$1*12,$2,sum3[i]/looptime,sum4[i]/looptime,sum5[i]/looptime,sum6[i]/looptime,sum7[i]/looptime,sum8[i]/looptime,sum9[i]/looptime,sum10[i]/looptime);
		}}}
  
}                                                                                                           
END {
}
