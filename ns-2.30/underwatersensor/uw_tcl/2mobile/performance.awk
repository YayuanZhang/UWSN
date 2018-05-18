BEGIN {
	numsend=0;
	numrecv=0;
        cmudelay=0;
        count=0;
}
{

   if ($1=="datarate")
        {   
		printf("%f ",$2);
	}
   if ($1=="datarate_bct")
        {   
		printf("%f ",$2);
	}

  if ($1=="SINK" && $2!=12 && $2!=13)
	{
		numsend+=$4;
	}
  if ($1=="SINK" && ($2==12 || $2==13))
	{
              numrecv+=$6;
              cmudelay+=$8
	}
     if ($1=="SINK" && $2==13)
	{       
		printf("%f ", numsend*300*8/1000/2400);
		printf("%d ", numsend);
                printf("%d ", numrecv);
		printf("%f ", cmudelay);
                printf("%f ", numrecv*300*8/1000/2400);
                printf("%f ", cmudelay/numrecv);
                printf("%f ", numrecv/numsend);
	}
      if ($1=="God")
        {
		printf("%f ", $3);
                printf("%f \n",$3/numrecv);
        } 
}                                                                                                           
END {

}
