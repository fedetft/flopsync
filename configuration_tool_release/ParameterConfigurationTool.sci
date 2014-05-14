clear; clc;

//
// Conf tool core functions
//

// Considering a system that synchronises every T [s], with a tuning fork XTAL
// whose turnover temperature is tho [°C] and parabolic temperature coefficient
// beta [ppm/°C^2], subeject to an exponential temperature change from tmn [°C]
// to tmx [°C] and vice versa, with maximum temperature change rate rtheta
// [°C/s], compute the clock synchronisation disturbance at the end of every
// sync period (hence, a discrete set of disturbances every T seconds) during
// the thermal transient from tmn to tmx (dsp [s]), and from tmx to tmn (dsm [s])
function [dsp,dsm]=clockDisturbance(T,tho,cbeta,tmn,tmx,rtheta)
    dsp    = [];
    dsm    = [];
    kmax   = ceil((5*((tmx-tmn)/rtheta))/T);
    for k=0:kmax
        dp  = (%e^(-(2*k*rtheta*T)/(tmx-tmn)-(2*rtheta*T)/(tmx-tmn))...
              *(((2*k+2)*rtheta*tho^2+(-4*k-4)*rtheta*tmx*tho+(2*k+2)...
              *rtheta*tmx^2)*T*%e^((2*k*rtheta*T)/(tmx-tmn)+(2*rtheta...
              *T)/(tmx-tmn))+((-4*tmx^2+8*tmn*tmx-4*tmn^2)*tho...
              +4*tmx^3-8*tmn*tmx^2+4*tmn^2*tmx)*%e^((k*rtheta*T)...
              /(tmx-tmn)+(rtheta*T)/(tmx-tmn))-tmx^3+3*tmn*tmx...
              ^2-3*tmn^2*tmx+tmn^3))/(2*rtheta)-(%e^(-(2*k*rtheta*T)...
              /(tmx-tmn))*((2*k*rtheta*tho^2-4*k*rtheta*tmx*tho+2*k...
              *rtheta*tmx^2)*T*%e^((2*k*rtheta*T)/(tmx-tmn))+((-4...
              *tmx^2+8*tmn*tmx-4*tmn^2)*tho+4*tmx^3-8*tmn*tmx...
              ^2+4*tmn^2*tmx)*%e^((k*rtheta*T)/(tmx-tmn))-tmx^3...
              +3*tmn*tmx^2-3*tmn^2*tmx+tmn^3))/(2*rtheta);
        dm  = (%e^(-(2*k*rtheta*T)/(tmx-tmn)-(2*rtheta*T)/(tmx-tmn))...
              *((2*rtheta*tho^2-4*rtheta*tmn*tho+2*rtheta*tmn^2)*T...
              *%e^((2*k*rtheta*T)/(tmx-tmn)+(2*rtheta*T)/(tmx-tmn))...
              +(((-4*tmx^2+8*tmn*tmx-4*tmn^2)*tho+4*tmn*tmx^2...
              -8*tmn^2*tmx+4*tmn^3)*%e^((2*rtheta*T)/(tmx-tmn))...
              +((4*tmx^2-8*tmn*tmx+4*tmn^2)*tho-4*tmn*tmx^2+8...
              *tmn^2*tmx-4*tmn^3)*%e^((rtheta*T)/(tmx-tmn)))...
              *%e^((k*rtheta*T)/(tmx-tmn))+(tmx^3-3*tmn*tmx^2...
              +3*tmn^2*tmx-tmn^3)*%e^((2*rtheta*T)/(tmx-tmn))...
              -tmx^3+3*tmn*tmx^2-3*tmn^2*tmx+tmn^3))/(2*rtheta);
        dsp = [dsp,-cbeta/1e6*dp]; 
        dsm = [dsm,-cbeta/1e6*dm]; 
    end
endfunction

// Given a disturbance d [s], and FLOPSYNC controller
// R(z)=3(1-alpha)z^2+3(1-alpha^2)z+1-alpha^3/(z-1)^2 with a given alpha [1]
// compute the synchronization error e [s]
function e=clockError(d,alpha)
    e=dsimul(tf2ss((%z-1)^2/(%z-alpha)^3),d-d(1));
endfunction

//
// Conf tool main code
//

param=fscanfMat('param.dat');
params=[];
for i=1:length(param)
    s="%.0f";
    if(i==2) then s="%.3f"; end
    if(i==5) then s="%.2f"; end
    params=[params;sprintf(s,param(i))];
end
params=x_mdialog('Flopsync configuration parameter selection',...
  ['XTAL turnover temperature (°C)';...
   'XTAL beta (ppm/°C^2)';...
   'Minimum ambient temperature (°C)';...
   'Maximum ambient temperature (°C)';...
   'Maximum temperature change rate (°C/min)';...
   'Maximum short term temperature change (°C)';...
   'Simulate positive/negative/both temperature transients (0/1/2)';...
   'Maximum tolerable synchronization error (us)';...
   'Acceptable error threshold after a temperature change (us)';...
   'Time to recover within acceptable error after a temperature change (s)'],...
   params);

for i=1:length(param)
    param(i)=msscanf(params(i),"%f");
end
fprintfMat('param.dat',param);

wb=waitbar('please wait');

tho      =  param(1);
cbeta    =  param(2);
themn    =  param(3);    
themx    =  param(4);
rtheta   =  param(5)/60;
dtheta   =  param(6);
which    =  param(7);
ebar     =  param(9)*1e-6;
vT       =  5:1:90;       //A reasonable range for synchronisation periods
va       =  0.1:0.01:0.6; //Capped to 0.6 as for higher values the H2 norm increases
mmerrMax =  param(8)*1e-6;
mtrecMax =  param(10);


tests_m = [themn,themx-dtheta];
tests_x = [themn+dtheta,themx];
mmerr   = zeros(length(vT),length(va));
mtrec   = zeros(length(vT),length(va));

incr=0;
for a=1:length(tests_m)
    tmn=tests_m(a);
    tmx=tests_x(a);
    for i=1:length(vT);
        incr=incr+1;
        waitbar(incr/(length(tests_m)*length(vT)),wb);
        T      = vT(i);
        // Compute disturbances
        [dsp,dsm]=clockDisturbance(T,tho,cbeta,tmn,tmx,rtheta);
        // compute errors
        for j=1:length(va)
            a          = va(j);
            ep         = clockError(dsp,a);
            em         = clockError(dsm,a);
            if which==0 then
                maxerr = max(abs(ep));
            elseif which==1 then
                maxerr = max(abs(em));
            else
                maxerr = max(max(abs(ep)),max(abs(em)));
            end
            mT(i,j)    = T;
            ma(i,j)    = a;
            mmerr(i,j) = max(mmerr(i,j),maxerr);
            dexp       = find(abs(ep)>ebar);
            if length(dexp>=1) then
               dexp    = dexp($);
            else
               dexp    = 1;
            end
            dexm       = find(abs(em)>ebar);
            if length(dexm>=1) then
               dexm    = dexm($);
            else
               dexm    = 1;
            end
            if which==0 then
                mtrec(i,j) = max(mtrec(i,j),dexp*T);
            elseif which==1 then
                mtrec(i,j) = max(mtrec(i,j),dexm*T);
            else
                mtrec(i,j) = max(mtrec(i,j),max(dexp,dexm)*T);
            end
        end
    end
end
close(wb);

hf=scf(1); clf;
title("Maximum synchronization error as a function of the controller parameters");
hf.figure_size = [600,600];
hf.color_map = summercolormap(16);
   surf(mT,ma,mmerr*1e6);
   ax=gca(); ax.log_flags="nnl"; ax.font_size=2;
   xlabel("$T\;[s]$","fontsize",4,...
           "position",[mean(vT),min(va)-0.35*(max(va)-min(va))]);
   ylabel("$\alpha$","fontsize",4,...
          "position",[min(vT)-0.35*(max(vT)-min(vT)),mean(va)]);
   zlabel("$e_{max}\;[\mu s]$","fontsize",4,"rotation",90);
//xs2png(1,sprintf('tempDist-surfMaxErr.png'));

hf=scf(2); clf;
title("Time to recover within acceptable bounds as a function of the controller parameters");
hf.figure_size = [600,600];
hf.color_map = summercolormap(16);
   surf(mT,ma,mtrec);
   ax=gca(); ax.log_flags="nnl"; ax.font_size=2;
   xlabel("$T\;[s]$","fontsize",4,...
           "position",[mean(vT),min(va)-0.35*(max(va)-min(va))]);
   ylabel("$\alpha$","fontsize",4,...
          "position",[min(vT)-0.35*(max(vT)-min(vT)),mean(va)]);
   zlabel("$t_{r,max}\;[s]$","fontsize",4,"rotation",90);
//xs2png(2,sprintf('tempDist-surfTrec.png'));


hf=scf(3); clf;
title("Controller parameters meeting the design requirements");
hf.figure_size = [600,600];
//hf.color_map = whitecolormap(16);
M = %nan*ones(mmerr); M (find(mmerr<mmerrMax & mtrec<mtrecMax)) = 1;
mesh(mT,ma,M,'facecol','green','edgecol','green');
ax = gca(); ax.rotation_angles=[0,-90];
ax.tight_limits="on";
//ax.x_ticks.labels="";
ax.z_ticks.labels="";
ax.title.position=[min(vT)+0.08*(max(vT)-min(vT)),1.02*max(va)];

va=ma(1,:);
vT=mT(:,1)';
mm = zeros(mmerr); mm(find(mmerr<mmerrMax & mtrec<mtrecMax)) = 1;
xset('color',3);
xset('thickness',3);
xset("fpf"," ")
contour(vT,va,mm,[0,0.9]);
xset('thickness',1);
xset('color',0);


// title(sprintf("$e_{max}=%d \,\mu s,\; t_{r,max}=%d\, s$",...
//   mmerrMax*1e6,mtrecMax),"fontsize",4);
xlabel("T"); ylabel("$\alpha$","fontsize",4);zlabel("");
//xs2png(3,sprintf('tempDist-alphaT-.png'));
