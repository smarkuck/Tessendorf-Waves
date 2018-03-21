### Implementation of J. Tessendorf's paper [_"Simulating Ocean Water"_](http://graphics.ucsd.edu/courses/rendering/2005/jdewall/tessendorf.pdf).

This program is able to simulate water waves in real time. Application creates Philipps spectrum and then, compute wave heights with reverse FFT.  I included only 32 bit dependencies, so project cannot be compiled in 64bit.

#### controls
w,a,s,d - move camera<br/>
q/e - move camera down/up<br/>
1 - skybox on/off<br/>
2 - polygon mode line/fill<br/>
3 - enable/disable sound<br/>
4/5 - decrease/increase wind speed<br/>
6/7 - decrease/increase waves height<br/>
9/0 - decrease/increase waves quality<br/>
-/+ - decrease/increase view range


