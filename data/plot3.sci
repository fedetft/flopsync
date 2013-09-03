clear; clc; clf 
stacksize(100000000);

a=fscanfMat('node1.dat');
[q,w]=size(a);
d=a(:,5:$);

x=[];
for i=1:q
	x=[x,d(i,:)];
end
t=1:length(x);
t=t./(w-4);

e=a(:,1);
y=[0];
for i=2:q
	y=[y,e(i)];
end

tt=0:length(e)-1;
plot(t,x,'b');
plot(tt,-y,'.r');
ax=get("current_axes");
ax.data_bounds=[0,-2000;length(e),2000];
xlabel("Time (min)");
ylabel("Sync error (us)");

settling=15;
xx=x(settling*(w-4):$);
printf("\nMean               (comb data) =%f\n",nanmean(xx));
printf("Standard deviation (comb data) =%f\n",nanstdev(xx));
yy=-y(settling:$);
printf("Mean               (e(t(k+1))-)=%f\n",nanmean(yy));
printf("Standard deviation (e(t(k+1))-)=%f\n",nanstdev(yy));