/* -*- mode: c; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4; c-file-style: "stroustrup"; -*- 
 *
 * 
 * This file is part of Gromacs        Copyright (c) 1991-2004
 * David van der Spoel, Erik Lindahl, University of Groningen.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * To help us fund GROMACS development, we humbly ask that you cite
 * the research papers on the package. Check out http://www.gromacs.org
 * 
 * And Hey:
 * Gnomes, ROck Monsters And Chili Sauce
 */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

/* Must come directly after config.h */
#ifdef GMX_THREAD_SHM_FDECOMP
#include <thread_mpi.h>
#endif


#include "ppc_altivec_util.h"
#include "nb_kernel104_ppc_altivec.h"



void 
nb_kernel104_ppc_altivec  (int *             p_nri,
                       int               iinr[],
                       int               jindex[],
                       int               jjnr[],
                       int               shift[],
                       float             shiftvec[],
                       float             fshift[],
                       int               gid[],
                       float             pos[],
                       float             faction[],
                       float             charge[],
                       float *           p_facel,
                       float *           p_krf,
                       float *           p_crf,
                       float             Vc[],
                       int               type[],
                       int *             p_ntype,
                       float             vdwparam[],
                       float             Vvdw[],
                       float *           p_tabscale,
                       float             VFtab[],
                       float             invsqrta[],
                       float             dvda[],
                       float *           p_gbtabscale,
                       float             GBtab[],
                       int *             p_nthreads,
                       int *             count,
                       void *            mtx,
                       int *             outeriter,
                       int *             inneriter,
					   float *           work)
{
	vector float ix1,iy1,iz1,ix2,iy2,iz2,ix3,iy3,iz3;
	vector float jx1,jy1,jz1,jx2,jy2,jz2,jx3,jy3,jz3;

	vector float dx11,dy11,dz11,dx12,dy12,dz12,dx13,dy13,dz13;
	vector float dx21,dy21,dz21,dx22,dy22,dz22,dx23,dy23,dz23;
	vector float dx31,dy31,dz31,dx32,dy32,dz32,dx33,dy33,dz33;

	vector float rsq11,rsq12,rsq13,rsq21,rsq22,rsq23,rsq31,rsq32,rsq33;
	vector float rinv11,rinv12,rinv13,rinv21,rinv22,rinv23;
	vector float rinv31,rinv32,rinv33;
	vector float rinvsq11,rinvsq12,rinvsq13;
	vector float rinvsq21,rinvsq22,rinvsq23;
	vector float rinvsq31,rinvsq32,rinvsq33;
	vector float vc11,vc12,vc13,vc21,vc22,vc23,vc31,vc32,vc33;

	vector float vfacel,nul;
	vector float fs11,fs12,fs13,fs21,fs22,fs23,fs31,fs32,fs33;
	vector float fix1,fiy1,fiz1,fix2,fiy2,fiz2,fix3,fiy3,fiz3;
	vector float fjx1,fjy1,fjz1,fjx2,fjy2,fjz2,fjx3,fjy3,fjz3;
	vector float vctot,qqMM,qqMH,qqHH,qM,qH,qqMMt,qqMHt,qqHHt; 

	int n,k,ii,is3,ii3,nj0,nj1;
	int jnra,jnrb,jnrc,jnrd;
	int j3a,j3b,j3c,j3d;
	int nri, ntype, nouter, ninner;
#ifdef GMX_THREAD_SHM_FDECOMP
	int nn0, nn1;
#endif

    nouter   = 0;
    ninner   = 0;
    nri      = *p_nri;
    ntype    = *p_ntype;
	nul=vec_zero();
	vfacel=load_float_and_splat(p_facel);
	qH        = load_float_and_splat(charge+iinr[0]+1);
	qM        = load_float_and_splat(charge+iinr[0]+3);
	qqMM      = vec_madd(qM,qM,nul);
	qqMH      = vec_madd(qM,qH,nul);
	qqHH      = vec_madd(qH,qH,nul);
	qqMM      = vec_madd(qqMM,vfacel,nul);
	qqMH      = vec_madd(qqMH,vfacel,nul);
	qqHH      = vec_madd(qqHH,vfacel,nul);

#ifdef GMX_THREAD_SHM_FDECOMP
    nthreads = *p_nthreads;
	do {
		tMPI_Thread_mutex_lock((tMPI_Thread_mutex_t *)mtx);
		nn0              = *count;
		nn1              = nn0+(nri-nn0)/(2*nthreads)+3;
		*count           = nn1;
		tMPI_Thread_mutex_unlock((tMPI_Thread_mutex_t *)mtx);
		if(nn1>nri) nn1=nri;
		for(n=nn0; (n<nn1); n++) {      
#if 0
		} /* maintain correct indentation even with conditional left braces */
#endif
#else /* without tMPI_Threads */
		for(n=0;n<nri;n++) {
#endif  
			is3        = 3*shift[n];
			ii         = iinr[n];
			ii3        = 3*ii;
			load_1_3atoms_shift_and_splat(pos+ii3+3,shiftvec+is3,
										  &ix1,&iy1,&iz1,
										  &ix2,&iy2,&iz2,&ix3,&iy3,&iz3);
			vctot      = nul;
			fix1       = nul;
			fiy1       = nul;
			fiz1       = nul;
			fix2       = nul;
			fiy2       = nul;
			fiz2       = nul;
			fix3       = nul;
			fiy3       = nul;
			fiz3       = nul;
			nj0        = jindex[n];
			nj1        = jindex[n+1];

			for(k=nj0; k<(nj1-3); k+=4) {
				jnra            = jjnr[k];
				jnrb            = jjnr[k+1];
				jnrc            = jjnr[k+2];
				jnrd            = jjnr[k+3];
				j3a             = 3*jnra;
				j3b             = 3*jnrb;
				j3c             = 3*jnrc;
				j3d             = 3*jnrd;
				load_4_3atoms(pos+j3a+3,pos+j3b+3,pos+j3c+3,pos+j3d+3,
							  &jx1,&jy1,&jz1,&jx2,&jy2,&jz2,&jx3,&jy3,&jz3);

				dx11            = vec_sub(ix1,jx1);
				dx12            = vec_sub(ix1,jx2);
				dx13            = vec_sub(ix1,jx3);
				dy11            = vec_sub(iy1,jy1);
				dy12            = vec_sub(iy1,jy2);
				dy13            = vec_sub(iy1,jy3);
				dz11            = vec_sub(iz1,jz1);
				dz12            = vec_sub(iz1,jz2);
				dz13            = vec_sub(iz1,jz3);
				dx21            = vec_sub(ix2,jx1);
				dx22            = vec_sub(ix2,jx2);
				dx23            = vec_sub(ix2,jx3);
				dy21            = vec_sub(iy2,jy1);
				dy22            = vec_sub(iy2,jy2);
				dy23            = vec_sub(iy2,jy3);
				dz21            = vec_sub(iz2,jz1);
				dz22            = vec_sub(iz2,jz2);
				dz23            = vec_sub(iz2,jz3);
				dx31            = vec_sub(ix3,jx1);
				dx32            = vec_sub(ix3,jx2);
				dx33            = vec_sub(ix3,jx3);
				dy31            = vec_sub(iy3,jy1);
				dy32            = vec_sub(iy3,jy2);
				dy33            = vec_sub(iy3,jy3);
				dz31            = vec_sub(iz3,jz1);
				dz32            = vec_sub(iz3,jz2);
				dz33            = vec_sub(iz3,jz3);

				rsq11           = vec_madd(dx11,dx11,nul);
				rsq12           = vec_madd(dx12,dx12,nul);
				rsq13           = vec_madd(dx13,dx13,nul);
				rsq21           = vec_madd(dx21,dx21,nul);
				rsq22           = vec_madd(dx22,dx22,nul);
				rsq23           = vec_madd(dx23,dx23,nul);
				rsq31           = vec_madd(dx31,dx31,nul);
				rsq32           = vec_madd(dx32,dx32,nul);
				rsq33           = vec_madd(dx33,dx33,nul);
				rsq11           = vec_madd(dy11,dy11,rsq11);
				rsq12           = vec_madd(dy12,dy12,rsq12);
				rsq13           = vec_madd(dy13,dy13,rsq13);
				rsq21           = vec_madd(dy21,dy21,rsq21);
				rsq22           = vec_madd(dy22,dy22,rsq22);
				rsq23           = vec_madd(dy23,dy23,rsq23);
				rsq31           = vec_madd(dy31,dy31,rsq31);
				rsq32           = vec_madd(dy32,dy32,rsq32);
				rsq33           = vec_madd(dy33,dy33,rsq33);
				rsq11           = vec_madd(dz11,dz11,rsq11);
				rsq12           = vec_madd(dz12,dz12,rsq12);
				rsq13           = vec_madd(dz13,dz13,rsq13);
				rsq21           = vec_madd(dz21,dz21,rsq21);
				rsq22           = vec_madd(dz22,dz22,rsq22);
				rsq23           = vec_madd(dz23,dz23,rsq23);
				rsq31           = vec_madd(dz31,dz31,rsq31);
				rsq32           = vec_madd(dz32,dz32,rsq32);
				rsq33           = vec_madd(dz33,dz33,rsq33);

				do_9_invsqrt(rsq11,rsq12,rsq13,
							 rsq21,rsq22,rsq23,
							 rsq31,rsq32,rsq33,
							 &rinv11,&rinv12,&rinv13,
							 &rinv21,&rinv22,&rinv23,
							 &rinv31,&rinv32,&rinv33);

				rinvsq11        = vec_madd(rinv11,rinv11,nul);
				rinvsq12        = vec_madd(rinv12,rinv12,nul);
				rinvsq13        = vec_madd(rinv13,rinv13,nul);
				rinvsq21        = vec_madd(rinv21,rinv21,nul);
				rinvsq22        = vec_madd(rinv22,rinv22,nul);
				rinvsq23        = vec_madd(rinv23,rinv23,nul);
				rinvsq31        = vec_madd(rinv31,rinv31,nul);
				rinvsq32        = vec_madd(rinv32,rinv32,nul);
				rinvsq33        = vec_madd(rinv33,rinv33,nul);

				vc11            = vec_madd(rinv11,qqHH,nul);
				vc12            = vec_madd(rinv12,qqHH,nul);
				vc13            = vec_madd(rinv13,qqMH,nul);
				vc21            = vec_madd(rinv21,qqHH,nul);
				vc22            = vec_madd(rinv22,qqHH,nul);
				vc23            = vec_madd(rinv23,qqMH,nul);
				vc31            = vec_madd(rinv31,qqMH,nul);
				vc32            = vec_madd(rinv32,qqMH,nul);
				vc33            = vec_madd(rinv33,qqMM,nul);

				fs11            = vec_madd(vc11,rinvsq11,nul);
				fs12            = vec_madd(vc12,rinvsq12,nul);
				fs13            = vec_madd(vc13,rinvsq13,nul);
				fs21            = vec_madd(vc21,rinvsq21,nul);
				fs22            = vec_madd(vc22,rinvsq22,nul);
				fs23            = vec_madd(vc23,rinvsq23,nul);
				fs31            = vec_madd(vc31,rinvsq31,nul);
				fs32            = vec_madd(vc32,rinvsq32,nul);
				fs33            = vec_madd(vc33,rinvsq33,nul);

				vctot           = vec_add(vctot,vc11);
				vc12            = vec_add(vc12,vc13);
				vc21            = vec_add(vc21,vc22);
				vc23            = vec_add(vc23,vc31);
				vc32            = vec_add(vc32,vc33);
				vctot           = vec_add(vctot,vc12);
				vc21            = vec_add(vc21,vc23);
				vctot           = vec_add(vctot,vc32);
				vctot           = vec_add(vctot,vc21); 

				fix1            = vec_madd(fs11,dx11,fix1);
				fiy1            = vec_madd(fs11,dy11,fiy1);
				fiz1            = vec_madd(fs11,dz11,fiz1);
				fix2            = vec_madd(fs21,dx21,fix2);
				fiy2            = vec_madd(fs21,dy21,fiy2);
				fiz2            = vec_madd(fs21,dz21,fiz2);
				fix3            = vec_madd(fs31,dx31,fix3);
				fiy3            = vec_madd(fs31,dy31,fiy3);
				fiz3            = vec_madd(fs31,dz31,fiz3);

				fix1            = vec_madd(fs12,dx12,fix1);
				fiy1            = vec_madd(fs12,dy12,fiy1);
				fiz1            = vec_madd(fs12,dz12,fiz1);
				fix2            = vec_madd(fs22,dx22,fix2);
				fiy2            = vec_madd(fs22,dy22,fiy2);
				fiz2            = vec_madd(fs22,dz22,fiz2);
				fix3            = vec_madd(fs32,dx32,fix3);
				fiy3            = vec_madd(fs32,dy32,fiy3);
				fiz3            = vec_madd(fs32,dz32,fiz3);

				fix1            = vec_madd(fs13,dx13,fix1);
				fiy1            = vec_madd(fs13,dy13,fiy1);
				fiz1            = vec_madd(fs13,dz13,fiz1);
				fix2            = vec_madd(fs23,dx23,fix2);
				fiy2            = vec_madd(fs23,dy23,fiy2);
				fiz2            = vec_madd(fs23,dz23,fiz2);
				fix3            = vec_madd(fs33,dx33,fix3);
				fiy3            = vec_madd(fs33,dy33,fiy3);
				fiz3            = vec_madd(fs33,dz33,fiz3);

				fjx1            = vec_nmsub(fs11,dx11,nul);
				fjy1            = vec_nmsub(fs11,dy11,nul);
				fjz1            = vec_nmsub(fs11,dz11,nul);
				fjx2            = vec_nmsub(fs12,dx12,nul);
				fjy2            = vec_nmsub(fs12,dy12,nul);
				fjz2            = vec_nmsub(fs12,dz12,nul);
				fjx3            = vec_nmsub(fs13,dx13,nul);
				fjy3            = vec_nmsub(fs13,dy13,nul);
				fjz3            = vec_nmsub(fs13,dz13,nul);

				fjx1            = vec_nmsub(fs21,dx21,fjx1);
				fjy1            = vec_nmsub(fs21,dy21,fjy1);
				fjz1            = vec_nmsub(fs21,dz21,fjz1);
				fjx2            = vec_nmsub(fs22,dx22,fjx2);
				fjy2            = vec_nmsub(fs22,dy22,fjy2);
				fjz2            = vec_nmsub(fs22,dz22,fjz2);
				fjx3            = vec_nmsub(fs23,dx23,fjx3);
				fjy3            = vec_nmsub(fs23,dy23,fjy3);
				fjz3            = vec_nmsub(fs23,dz23,fjz3);

				fjx1            = vec_nmsub(fs31,dx31,fjx1);
				fjy1            = vec_nmsub(fs31,dy31,fjy1);
				fjz1            = vec_nmsub(fs31,dz31,fjz1);
				fjx2            = vec_nmsub(fs32,dx32,fjx2);
				fjy2            = vec_nmsub(fs32,dy32,fjy2);
				fjz2            = vec_nmsub(fs32,dz32,fjz2);
				fjx3            = vec_nmsub(fs33,dx33,fjx3);
				fjy3            = vec_nmsub(fs33,dy33,fjy3);
				fjz3            = vec_nmsub(fs33,dz33,fjz3);

				add_force_to_4_3atoms(faction+j3a+3,faction+j3b+3,
									  faction+j3c+3,faction+j3d+3,
									  fjx1,fjy1,fjz1,fjx2,fjy2,fjz2,
									  fjx3,fjy3,fjz3);
			} 
			if(k<(nj1-2)) {
				jnra            = jjnr[k];
				jnrb            = jjnr[k+1];
				jnrc            = jjnr[k+2];
				j3a             = 3*jnra;
				j3b             = 3*jnrb;
				j3c             = 3*jnrc;
				load_3_3atoms(pos+j3a+3,pos+j3b+3,pos+j3c+3,
							  &jx1,&jy1,&jz1,&jx2,&jy2,&jz2,&jx3,&jy3,&jz3);
				qqMMt           = vec_sld(qqMM,nul,4);
				qqMHt           = vec_sld(qqMH,nul,4);
				qqHHt           = vec_sld(qqHH,nul,4);

				dx11            = vec_sub(ix1,jx1);
				dx12            = vec_sub(ix1,jx2);
				dx13            = vec_sub(ix1,jx3);
				dy11            = vec_sub(iy1,jy1);
				dy12            = vec_sub(iy1,jy2);
				dy13            = vec_sub(iy1,jy3);
				dz11            = vec_sub(iz1,jz1);
				dz12            = vec_sub(iz1,jz2);
				dz13            = vec_sub(iz1,jz3);
				dx21            = vec_sub(ix2,jx1);
				dx22            = vec_sub(ix2,jx2);
				dx23            = vec_sub(ix2,jx3);
				dy21            = vec_sub(iy2,jy1);
				dy22            = vec_sub(iy2,jy2);
				dy23            = vec_sub(iy2,jy3);
				dz21            = vec_sub(iz2,jz1);
				dz22            = vec_sub(iz2,jz2);
				dz23            = vec_sub(iz2,jz3);
				dx31            = vec_sub(ix3,jx1);
				dx32            = vec_sub(ix3,jx2);
				dx33            = vec_sub(ix3,jx3);
				dy31            = vec_sub(iy3,jy1);
				dy32            = vec_sub(iy3,jy2);
				dy33            = vec_sub(iy3,jy3);
				dz31            = vec_sub(iz3,jz1);
				dz32            = vec_sub(iz3,jz2);
				dz33            = vec_sub(iz3,jz3);

				rsq11           = vec_madd(dx11,dx11,nul);
				rsq12           = vec_madd(dx12,dx12,nul);
				rsq13           = vec_madd(dx13,dx13,nul);
				rsq21           = vec_madd(dx21,dx21,nul);
				rsq22           = vec_madd(dx22,dx22,nul);
				rsq23           = vec_madd(dx23,dx23,nul);
				rsq31           = vec_madd(dx31,dx31,nul);
				rsq32           = vec_madd(dx32,dx32,nul);
				rsq33           = vec_madd(dx33,dx33,nul);
				rsq11           = vec_madd(dy11,dy11,rsq11);
				rsq12           = vec_madd(dy12,dy12,rsq12);
				rsq13           = vec_madd(dy13,dy13,rsq13);
				rsq21           = vec_madd(dy21,dy21,rsq21);
				rsq22           = vec_madd(dy22,dy22,rsq22);
				rsq23           = vec_madd(dy23,dy23,rsq23);
				rsq31           = vec_madd(dy31,dy31,rsq31);
				rsq32           = vec_madd(dy32,dy32,rsq32);
				rsq33           = vec_madd(dy33,dy33,rsq33);
				rsq11           = vec_madd(dz11,dz11,rsq11);
				rsq12           = vec_madd(dz12,dz12,rsq12);
				rsq13           = vec_madd(dz13,dz13,rsq13);
				rsq21           = vec_madd(dz21,dz21,rsq21);
				rsq22           = vec_madd(dz22,dz22,rsq22);
				rsq23           = vec_madd(dz23,dz23,rsq23);
				rsq31           = vec_madd(dz31,dz31,rsq31);
				rsq32           = vec_madd(dz32,dz32,rsq32);
				rsq33           = vec_madd(dz33,dz33,rsq33);

				do_9_invsqrt(rsq11,rsq12,rsq13,
							 rsq21,rsq22,rsq23,
							 rsq31,rsq32,rsq33,
							 &rinv11,&rinv12,&rinv13,
							 &rinv21,&rinv22,&rinv23,
							 &rinv31,&rinv32,&rinv33);
      
				zero_highest_element_in_9_vectors(&rinv11,&rinv12,&rinv13,
												  &rinv21,&rinv22,&rinv23,
												  &rinv31,&rinv32,&rinv33);

				rinvsq11        = vec_madd(rinv11,rinv11,nul);
				rinvsq12        = vec_madd(rinv12,rinv12,nul);
				rinvsq13        = vec_madd(rinv13,rinv13,nul);
				rinvsq21        = vec_madd(rinv21,rinv21,nul);
				rinvsq22        = vec_madd(rinv22,rinv22,nul);
				rinvsq23        = vec_madd(rinv23,rinv23,nul);
				rinvsq31        = vec_madd(rinv31,rinv31,nul);
				rinvsq32        = vec_madd(rinv32,rinv32,nul);
				rinvsq33        = vec_madd(rinv33,rinv33,nul);

				vc11            = vec_madd(rinv11,qqHHt,nul);
				vc12            = vec_madd(rinv12,qqHHt,nul);
				vc13            = vec_madd(rinv13,qqMHt,nul);
				vc21            = vec_madd(rinv21,qqHHt,nul);
				vc22            = vec_madd(rinv22,qqHHt,nul);
				vc23            = vec_madd(rinv23,qqMHt,nul);
				vc31            = vec_madd(rinv31,qqMHt,nul);
				vc32            = vec_madd(rinv32,qqMHt,nul);
				vc33            = vec_madd(rinv33,qqMMt,nul);
      
				fs11            = vec_madd(vc11,rinvsq11,nul);
				fs12            = vec_madd(vc12,rinvsq12,nul);
				fs13            = vec_madd(vc13,rinvsq13,nul);
				fs21            = vec_madd(vc21,rinvsq21,nul);
				fs22            = vec_madd(vc22,rinvsq22,nul);
				fs23            = vec_madd(vc23,rinvsq23,nul);
				fs31            = vec_madd(vc31,rinvsq31,nul);
				fs32            = vec_madd(vc32,rinvsq32,nul);
				fs33            = vec_madd(vc33,rinvsq33,nul);

				vctot           = vec_add(vctot,vc11);
				vc12            = vec_add(vc12,vc13);
				vc21            = vec_add(vc21,vc22);
				vc23            = vec_add(vc23,vc31);
				vc32            = vec_add(vc32,vc33);
				vctot           = vec_add(vctot,vc12);
				vc21            = vec_add(vc21,vc23);
				vctot           = vec_add(vctot,vc32);
				vctot           = vec_add(vctot,vc21); 

				fix1            = vec_madd(fs11,dx11,fix1);
				fiy1            = vec_madd(fs11,dy11,fiy1);
				fiz1            = vec_madd(fs11,dz11,fiz1);
				fix2            = vec_madd(fs21,dx21,fix2);
				fiy2            = vec_madd(fs21,dy21,fiy2);
				fiz2            = vec_madd(fs21,dz21,fiz2);
				fix3            = vec_madd(fs31,dx31,fix3);
				fiy3            = vec_madd(fs31,dy31,fiy3);
				fiz3            = vec_madd(fs31,dz31,fiz3);

				fix1            = vec_madd(fs12,dx12,fix1);
				fiy1            = vec_madd(fs12,dy12,fiy1);
				fiz1            = vec_madd(fs12,dz12,fiz1);
				fix2            = vec_madd(fs22,dx22,fix2);
				fiy2            = vec_madd(fs22,dy22,fiy2);
				fiz2            = vec_madd(fs22,dz22,fiz2);
				fix3            = vec_madd(fs32,dx32,fix3);
				fiy3            = vec_madd(fs32,dy32,fiy3);
				fiz3            = vec_madd(fs32,dz32,fiz3);

				fix1            = vec_madd(fs13,dx13,fix1);
				fiy1            = vec_madd(fs13,dy13,fiy1);
				fiz1            = vec_madd(fs13,dz13,fiz1);
				fix2            = vec_madd(fs23,dx23,fix2);
				fiy2            = vec_madd(fs23,dy23,fiy2);
				fiz2            = vec_madd(fs23,dz23,fiz2);
				fix3            = vec_madd(fs33,dx33,fix3);
				fiy3            = vec_madd(fs33,dy33,fiy3);
				fiz3            = vec_madd(fs33,dz33,fiz3);

				fjx1            = vec_nmsub(fs11,dx11,nul);
				fjy1            = vec_nmsub(fs11,dy11,nul);
				fjz1            = vec_nmsub(fs11,dz11,nul);
				fjx2            = vec_nmsub(fs12,dx12,nul);
				fjy2            = vec_nmsub(fs12,dy12,nul);
				fjz2            = vec_nmsub(fs12,dz12,nul);
				fjx3            = vec_nmsub(fs13,dx13,nul);
				fjy3            = vec_nmsub(fs13,dy13,nul);
				fjz3            = vec_nmsub(fs13,dz13,nul);

				fjx1            = vec_nmsub(fs21,dx21,fjx1);
				fjy1            = vec_nmsub(fs21,dy21,fjy1);
				fjz1            = vec_nmsub(fs21,dz21,fjz1);
				fjx2            = vec_nmsub(fs22,dx22,fjx2);
				fjy2            = vec_nmsub(fs22,dy22,fjy2);
				fjz2            = vec_nmsub(fs22,dz22,fjz2);
				fjx3            = vec_nmsub(fs23,dx23,fjx3);
				fjy3            = vec_nmsub(fs23,dy23,fjy3);
				fjz3            = vec_nmsub(fs23,dz23,fjz3);

				fjx1            = vec_nmsub(fs31,dx31,fjx1);
				fjy1            = vec_nmsub(fs31,dy31,fjy1);
				fjz1            = vec_nmsub(fs31,dz31,fjz1);
				fjx2            = vec_nmsub(fs32,dx32,fjx2);
				fjy2            = vec_nmsub(fs32,dy32,fjy2);
				fjz2            = vec_nmsub(fs32,dz32,fjz2);
				fjx3            = vec_nmsub(fs33,dx33,fjx3);
				fjy3            = vec_nmsub(fs33,dy33,fjy3);
				fjz3            = vec_nmsub(fs33,dz33,fjz3);

				add_force_to_3_3atoms(faction+j3a+3,faction+j3b+3,
									  faction+j3c+3,
									  fjx1,fjy1,fjz1,fjx2,fjy2,fjz2,
									  fjx3,fjy3,fjz3);
			} else if(k<(nj1-1)) {
				jnra            = jjnr[k];
				jnrb            = jjnr[k+1];
				j3a             = 3*jnra;
				j3b             = 3*jnrb;
				load_2_3atoms(pos+j3a+3,pos+j3b+3,
							  &jx1,&jy1,&jz1,&jx2,&jy2,&jz2,&jx3,&jy3,&jz3);
				qqMMt           = vec_sld(qqMM,nul,8);
				qqMHt           = vec_sld(qqMH,nul,8);
				qqHHt           = vec_sld(qqHH,nul,8);

				dx11            = vec_sub(ix1,jx1);
				dx12            = vec_sub(ix1,jx2);
				dx13            = vec_sub(ix1,jx3);
				dy11            = vec_sub(iy1,jy1);
				dy12            = vec_sub(iy1,jy2);
				dy13            = vec_sub(iy1,jy3);
				dz11            = vec_sub(iz1,jz1);
				dz12            = vec_sub(iz1,jz2);
				dz13            = vec_sub(iz1,jz3);
				dx21            = vec_sub(ix2,jx1);
				dx22            = vec_sub(ix2,jx2);
				dx23            = vec_sub(ix2,jx3);
				dy21            = vec_sub(iy2,jy1);
				dy22            = vec_sub(iy2,jy2);
				dy23            = vec_sub(iy2,jy3);
				dz21            = vec_sub(iz2,jz1);
				dz22            = vec_sub(iz2,jz2);
				dz23            = vec_sub(iz2,jz3);
				dx31            = vec_sub(ix3,jx1);
				dx32            = vec_sub(ix3,jx2);
				dx33            = vec_sub(ix3,jx3);
				dy31            = vec_sub(iy3,jy1);
				dy32            = vec_sub(iy3,jy2);
				dy33            = vec_sub(iy3,jy3);
				dz31            = vec_sub(iz3,jz1);
				dz32            = vec_sub(iz3,jz2);
				dz33            = vec_sub(iz3,jz3);

				rsq11           = vec_madd(dx11,dx11,nul);
				rsq12           = vec_madd(dx12,dx12,nul);
				rsq13           = vec_madd(dx13,dx13,nul);
				rsq21           = vec_madd(dx21,dx21,nul);
				rsq22           = vec_madd(dx22,dx22,nul);
				rsq23           = vec_madd(dx23,dx23,nul);
				rsq31           = vec_madd(dx31,dx31,nul);
				rsq32           = vec_madd(dx32,dx32,nul);
				rsq33           = vec_madd(dx33,dx33,nul);
				rsq11           = vec_madd(dy11,dy11,rsq11);
				rsq12           = vec_madd(dy12,dy12,rsq12);
				rsq13           = vec_madd(dy13,dy13,rsq13);
				rsq21           = vec_madd(dy21,dy21,rsq21);
				rsq22           = vec_madd(dy22,dy22,rsq22);
				rsq23           = vec_madd(dy23,dy23,rsq23);
				rsq31           = vec_madd(dy31,dy31,rsq31);
				rsq32           = vec_madd(dy32,dy32,rsq32);
				rsq33           = vec_madd(dy33,dy33,rsq33);
				rsq11           = vec_madd(dz11,dz11,rsq11);
				rsq12           = vec_madd(dz12,dz12,rsq12);
				rsq13           = vec_madd(dz13,dz13,rsq13);
				rsq21           = vec_madd(dz21,dz21,rsq21);
				rsq22           = vec_madd(dz22,dz22,rsq22);
				rsq23           = vec_madd(dz23,dz23,rsq23);
				rsq31           = vec_madd(dz31,dz31,rsq31);
				rsq32           = vec_madd(dz32,dz32,rsq32);
				rsq33           = vec_madd(dz33,dz33,rsq33);

				do_9_invsqrt(rsq11,rsq12,rsq13,
							 rsq21,rsq22,rsq23,
							 rsq31,rsq32,rsq33,
							 &rinv11,&rinv12,&rinv13,
							 &rinv21,&rinv22,&rinv23,
							 &rinv31,&rinv32,&rinv33);
      
				zero_highest_2_elements_in_9_vectors(&rinv11,&rinv12,&rinv13,
													 &rinv21,&rinv22,&rinv23,
													 &rinv31,&rinv32,&rinv33);

				rinvsq11        = vec_madd(rinv11,rinv11,nul);
				rinvsq12        = vec_madd(rinv12,rinv12,nul);
				rinvsq13        = vec_madd(rinv13,rinv13,nul);
				rinvsq21        = vec_madd(rinv21,rinv21,nul);
				rinvsq22        = vec_madd(rinv22,rinv22,nul);
				rinvsq23        = vec_madd(rinv23,rinv23,nul);
				rinvsq31        = vec_madd(rinv31,rinv31,nul);
				rinvsq32        = vec_madd(rinv32,rinv32,nul);
				rinvsq33        = vec_madd(rinv33,rinv33,nul);

				vc11            = vec_madd(rinv11,qqHHt,nul);
				vc12            = vec_madd(rinv12,qqHHt,nul);
				vc13            = vec_madd(rinv13,qqMHt,nul);
				vc21            = vec_madd(rinv21,qqHHt,nul);
				vc22            = vec_madd(rinv22,qqHHt,nul);
				vc23            = vec_madd(rinv23,qqMHt,nul);
				vc31            = vec_madd(rinv31,qqMHt,nul);
				vc32            = vec_madd(rinv32,qqMHt,nul);
				vc33            = vec_madd(rinv33,qqMMt,nul);
      
				fs11            = vec_madd(vc11,rinvsq11,nul);
				fs12            = vec_madd(vc12,rinvsq12,nul);
				fs13            = vec_madd(vc13,rinvsq13,nul);
				fs21            = vec_madd(vc21,rinvsq21,nul);
				fs22            = vec_madd(vc22,rinvsq22,nul);
				fs23            = vec_madd(vc23,rinvsq23,nul);
				fs31            = vec_madd(vc31,rinvsq31,nul);
				fs32            = vec_madd(vc32,rinvsq32,nul);
				fs33            = vec_madd(vc33,rinvsq33,nul);

				vctot           = vec_add(vctot,vc11);
				vc12            = vec_add(vc12,vc13);
				vc21            = vec_add(vc21,vc22);
				vc23            = vec_add(vc23,vc31);
				vc32            = vec_add(vc32,vc33);
				vctot           = vec_add(vctot,vc12);
				vc21            = vec_add(vc21,vc23);
				vctot           = vec_add(vctot,vc32);
				vctot           = vec_add(vctot,vc21); 

				fix1            = vec_madd(fs11,dx11,fix1);
				fiy1            = vec_madd(fs11,dy11,fiy1);
				fiz1            = vec_madd(fs11,dz11,fiz1);
				fix2            = vec_madd(fs21,dx21,fix2);
				fiy2            = vec_madd(fs21,dy21,fiy2);
				fiz2            = vec_madd(fs21,dz21,fiz2);
				fix3            = vec_madd(fs31,dx31,fix3);
				fiy3            = vec_madd(fs31,dy31,fiy3);
				fiz3            = vec_madd(fs31,dz31,fiz3);

				fix1            = vec_madd(fs12,dx12,fix1);
				fiy1            = vec_madd(fs12,dy12,fiy1);
				fiz1            = vec_madd(fs12,dz12,fiz1);
				fix2            = vec_madd(fs22,dx22,fix2);
				fiy2            = vec_madd(fs22,dy22,fiy2);
				fiz2            = vec_madd(fs22,dz22,fiz2);
				fix3            = vec_madd(fs32,dx32,fix3);
				fiy3            = vec_madd(fs32,dy32,fiy3);
				fiz3            = vec_madd(fs32,dz32,fiz3);

				fix1            = vec_madd(fs13,dx13,fix1);
				fiy1            = vec_madd(fs13,dy13,fiy1);
				fiz1            = vec_madd(fs13,dz13,fiz1);
				fix2            = vec_madd(fs23,dx23,fix2);
				fiy2            = vec_madd(fs23,dy23,fiy2);
				fiz2            = vec_madd(fs23,dz23,fiz2);
				fix3            = vec_madd(fs33,dx33,fix3);
				fiy3            = vec_madd(fs33,dy33,fiy3);
				fiz3            = vec_madd(fs33,dz33,fiz3);

				fjx1            = vec_nmsub(fs11,dx11,nul);
				fjy1            = vec_nmsub(fs11,dy11,nul);
				fjz1            = vec_nmsub(fs11,dz11,nul);
				fjx2            = vec_nmsub(fs12,dx12,nul);
				fjy2            = vec_nmsub(fs12,dy12,nul);
				fjz2            = vec_nmsub(fs12,dz12,nul);
				fjx3            = vec_nmsub(fs13,dx13,nul);
				fjy3            = vec_nmsub(fs13,dy13,nul);
				fjz3            = vec_nmsub(fs13,dz13,nul);

				fjx1            = vec_nmsub(fs21,dx21,fjx1);
				fjy1            = vec_nmsub(fs21,dy21,fjy1);
				fjz1            = vec_nmsub(fs21,dz21,fjz1);
				fjx2            = vec_nmsub(fs22,dx22,fjx2);
				fjy2            = vec_nmsub(fs22,dy22,fjy2);
				fjz2            = vec_nmsub(fs22,dz22,fjz2);
				fjx3            = vec_nmsub(fs23,dx23,fjx3);
				fjy3            = vec_nmsub(fs23,dy23,fjy3);
				fjz3            = vec_nmsub(fs23,dz23,fjz3);

				fjx1            = vec_nmsub(fs31,dx31,fjx1);
				fjy1            = vec_nmsub(fs31,dy31,fjy1);
				fjz1            = vec_nmsub(fs31,dz31,fjz1);
				fjx2            = vec_nmsub(fs32,dx32,fjx2);
				fjy2            = vec_nmsub(fs32,dy32,fjy2);
				fjz2            = vec_nmsub(fs32,dz32,fjz2);
				fjx3            = vec_nmsub(fs33,dx33,fjx3);
				fjy3            = vec_nmsub(fs33,dy33,fjy3);
				fjz3            = vec_nmsub(fs33,dz33,fjz3);

				add_force_to_2_3atoms(faction+j3a+3,faction+j3b+3,
									  fjx1,fjy1,fjz1,fjx2,fjy2,fjz2,
									  fjx3,fjy3,fjz3);
			} else if(k<nj1) {
				jnra            = jjnr[k];
				j3a             = 3*jnra;
				load_1_3atoms(pos+j3a+3,
							  &jx1,&jy1,&jz1,&jx2,&jy2,&jz2,&jx3,&jy3,&jz3);
				qqMMt           = vec_sld(qqMM,nul,12);
				qqMHt           = vec_sld(qqMH,nul,12);
				qqHHt           = vec_sld(qqHH,nul,12);

				dx11            = vec_sub(ix1,jx1);
				dx12            = vec_sub(ix1,jx2);
				dx13            = vec_sub(ix1,jx3);
				dy11            = vec_sub(iy1,jy1);
				dy12            = vec_sub(iy1,jy2);
				dy13            = vec_sub(iy1,jy3);
				dz11            = vec_sub(iz1,jz1);
				dz12            = vec_sub(iz1,jz2);
				dz13            = vec_sub(iz1,jz3);
				dx21            = vec_sub(ix2,jx1);
				dx22            = vec_sub(ix2,jx2);
				dx23            = vec_sub(ix2,jx3);
				dy21            = vec_sub(iy2,jy1);
				dy22            = vec_sub(iy2,jy2);
				dy23            = vec_sub(iy2,jy3);
				dz21            = vec_sub(iz2,jz1);
				dz22            = vec_sub(iz2,jz2);
				dz23            = vec_sub(iz2,jz3);
				dx31            = vec_sub(ix3,jx1);
				dx32            = vec_sub(ix3,jx2);
				dx33            = vec_sub(ix3,jx3);
				dy31            = vec_sub(iy3,jy1);
				dy32            = vec_sub(iy3,jy2);
				dy33            = vec_sub(iy3,jy3);
				dz31            = vec_sub(iz3,jz1);
				dz32            = vec_sub(iz3,jz2);
				dz33            = vec_sub(iz3,jz3);

				rsq11           = vec_madd(dx11,dx11,nul);
				rsq12           = vec_madd(dx12,dx12,nul);
				rsq13           = vec_madd(dx13,dx13,nul);
				rsq21           = vec_madd(dx21,dx21,nul);
				rsq22           = vec_madd(dx22,dx22,nul);
				rsq23           = vec_madd(dx23,dx23,nul);
				rsq31           = vec_madd(dx31,dx31,nul);
				rsq32           = vec_madd(dx32,dx32,nul);
				rsq33           = vec_madd(dx33,dx33,nul);
				rsq11           = vec_madd(dy11,dy11,rsq11);
				rsq12           = vec_madd(dy12,dy12,rsq12);
				rsq13           = vec_madd(dy13,dy13,rsq13);
				rsq21           = vec_madd(dy21,dy21,rsq21);
				rsq22           = vec_madd(dy22,dy22,rsq22);
				rsq23           = vec_madd(dy23,dy23,rsq23);
				rsq31           = vec_madd(dy31,dy31,rsq31);
				rsq32           = vec_madd(dy32,dy32,rsq32);
				rsq33           = vec_madd(dy33,dy33,rsq33);
				rsq11           = vec_madd(dz11,dz11,rsq11);
				rsq12           = vec_madd(dz12,dz12,rsq12);
				rsq13           = vec_madd(dz13,dz13,rsq13);
				rsq21           = vec_madd(dz21,dz21,rsq21);
				rsq22           = vec_madd(dz22,dz22,rsq22);
				rsq23           = vec_madd(dz23,dz23,rsq23);
				rsq31           = vec_madd(dz31,dz31,rsq31);
				rsq32           = vec_madd(dz32,dz32,rsq32);
				rsq33           = vec_madd(dz33,dz33,rsq33);

				do_9_invsqrt(rsq11,rsq12,rsq13,
							 rsq21,rsq22,rsq23,
							 rsq31,rsq32,rsq33,
							 &rinv11,&rinv12,&rinv13,
							 &rinv21,&rinv22,&rinv23,
							 &rinv31,&rinv32,&rinv33);
      
				zero_highest_3_elements_in_9_vectors(&rinv11,&rinv12,&rinv13,
													 &rinv21,&rinv22,&rinv23,
													 &rinv31,&rinv32,&rinv33);

				rinvsq11        = vec_madd(rinv11,rinv11,nul);
				rinvsq12        = vec_madd(rinv12,rinv12,nul);
				rinvsq13        = vec_madd(rinv13,rinv13,nul);
				rinvsq21        = vec_madd(rinv21,rinv21,nul);
				rinvsq22        = vec_madd(rinv22,rinv22,nul);
				rinvsq23        = vec_madd(rinv23,rinv23,nul);
				rinvsq31        = vec_madd(rinv31,rinv31,nul);
				rinvsq32        = vec_madd(rinv32,rinv32,nul);
				rinvsq33        = vec_madd(rinv33,rinv33,nul);

				vc11            = vec_madd(rinv11,qqHHt,nul);
				vc12            = vec_madd(rinv12,qqHHt,nul);
				vc13            = vec_madd(rinv13,qqMHt,nul);
				vc21            = vec_madd(rinv21,qqHHt,nul);
				vc22            = vec_madd(rinv22,qqHHt,nul);
				vc23            = vec_madd(rinv23,qqMHt,nul);
				vc31            = vec_madd(rinv31,qqMHt,nul);
				vc32            = vec_madd(rinv32,qqMHt,nul);
				vc33            = vec_madd(rinv33,qqMMt,nul);
      
				fs11            = vec_madd(vc11,rinvsq11,nul);
				fs12            = vec_madd(vc12,rinvsq12,nul);
				fs13            = vec_madd(vc13,rinvsq13,nul);
				fs21            = vec_madd(vc21,rinvsq21,nul);
				fs22            = vec_madd(vc22,rinvsq22,nul);
				fs23            = vec_madd(vc23,rinvsq23,nul);
				fs31            = vec_madd(vc31,rinvsq31,nul);
				fs32            = vec_madd(vc32,rinvsq32,nul);
				fs33            = vec_madd(vc33,rinvsq33,nul);

				vctot           = vec_add(vctot,vc11);
				vc12            = vec_add(vc12,vc13);
				vc21            = vec_add(vc21,vc22);
				vc23            = vec_add(vc23,vc31);
				vc32            = vec_add(vc32,vc33);
				vctot           = vec_add(vctot,vc12);
				vc21            = vec_add(vc21,vc23);
				vctot           = vec_add(vctot,vc32);
				vctot           = vec_add(vctot,vc21); 

				fix1            = vec_madd(fs11,dx11,fix1);
				fiy1            = vec_madd(fs11,dy11,fiy1);
				fiz1            = vec_madd(fs11,dz11,fiz1);
				fix2            = vec_madd(fs21,dx21,fix2);
				fiy2            = vec_madd(fs21,dy21,fiy2);
				fiz2            = vec_madd(fs21,dz21,fiz2);
				fix3            = vec_madd(fs31,dx31,fix3);
				fiy3            = vec_madd(fs31,dy31,fiy3);
				fiz3            = vec_madd(fs31,dz31,fiz3);

				fix1            = vec_madd(fs12,dx12,fix1);
				fiy1            = vec_madd(fs12,dy12,fiy1);
				fiz1            = vec_madd(fs12,dz12,fiz1);
				fix2            = vec_madd(fs22,dx22,fix2);
				fiy2            = vec_madd(fs22,dy22,fiy2);
				fiz2            = vec_madd(fs22,dz22,fiz2);
				fix3            = vec_madd(fs32,dx32,fix3);
				fiy3            = vec_madd(fs32,dy32,fiy3);
				fiz3            = vec_madd(fs32,dz32,fiz3);

				fix1            = vec_madd(fs13,dx13,fix1);
				fiy1            = vec_madd(fs13,dy13,fiy1);
				fiz1            = vec_madd(fs13,dz13,fiz1);
				fix2            = vec_madd(fs23,dx23,fix2);
				fiy2            = vec_madd(fs23,dy23,fiy2);
				fiz2            = vec_madd(fs23,dz23,fiz2);
				fix3            = vec_madd(fs33,dx33,fix3);
				fiy3            = vec_madd(fs33,dy33,fiy3);
				fiz3            = vec_madd(fs33,dz33,fiz3);

				fjx1            = vec_nmsub(fs11,dx11,nul);
				fjy1            = vec_nmsub(fs11,dy11,nul);
				fjz1            = vec_nmsub(fs11,dz11,nul);
				fjx2            = vec_nmsub(fs12,dx12,nul);
				fjy2            = vec_nmsub(fs12,dy12,nul);
				fjz2            = vec_nmsub(fs12,dz12,nul);
				fjx3            = vec_nmsub(fs13,dx13,nul);
				fjy3            = vec_nmsub(fs13,dy13,nul);
				fjz3            = vec_nmsub(fs13,dz13,nul);

				fjx1            = vec_nmsub(fs21,dx21,fjx1);
				fjy1            = vec_nmsub(fs21,dy21,fjy1);
				fjz1            = vec_nmsub(fs21,dz21,fjz1);
				fjx2            = vec_nmsub(fs22,dx22,fjx2);
				fjy2            = vec_nmsub(fs22,dy22,fjy2);
				fjz2            = vec_nmsub(fs22,dz22,fjz2);
				fjx3            = vec_nmsub(fs23,dx23,fjx3);
				fjy3            = vec_nmsub(fs23,dy23,fjy3);
				fjz3            = vec_nmsub(fs23,dz23,fjz3);

				fjx1            = vec_nmsub(fs31,dx31,fjx1);
				fjy1            = vec_nmsub(fs31,dy31,fjy1);
				fjz1            = vec_nmsub(fs31,dz31,fjz1);
				fjx2            = vec_nmsub(fs32,dx32,fjx2);
				fjy2            = vec_nmsub(fs32,dy32,fjy2);
				fjz2            = vec_nmsub(fs32,dz32,fjz2);
				fjx3            = vec_nmsub(fs33,dx33,fjx3);
				fjy3            = vec_nmsub(fs33,dy33,fjy3);
				fjz3            = vec_nmsub(fs33,dz33,fjz3);

				add_force_to_1_3atoms(faction+j3a+3,
									  fjx1,fjy1,fjz1,fjx2,fjy2,fjz2,
									  fjx3,fjy3,fjz3);
			}
			/* update outer data */
			update_i_3atoms_forces(faction+ii3+3,fshift+is3,
								   fix1,fiy1,fiz1,fix2,fiy2,fiz2,
								   fix3,fiy3,fiz3);

			add_vector_to_float(Vc+gid[n],vctot);
			ninner += nj1 - nj0;
		}
#ifdef GMX_THREAD_SHM_FDECOMP
		nouter += nn1 - nn0;
	} while (nn1<nri);
#else
	nouter = nri;
#endif
	*outeriter = nouter;
	*inneriter = ninner;
}



void 
nb_kernel104nf_ppc_altivec(int *             p_nri,
                       int               iinr[],
                       int               jindex[],
                       int               jjnr[],
                       int               shift[],
                       float             shiftvec[],
                       float             fshift[],
                       int               gid[],
                       float             pos[],
                       float             faction[],
                       float             charge[],
                       float *           p_facel,
                       float *           p_krf,
                       float *           p_crf,
                       float             Vc[],
                       int               type[],
                       int *             p_ntype,
                       float             vdwparam[],
                       float             Vvdw[],
                       float *           p_tabscale,
                       float             VFtab[],
                       float             invsqrta[],
                       float             dvda[],
                       float *           p_gbtabscale,
                       float             GBtab[],
                       int *             p_nthreads,
                       int *             count,
                       void *            mtx,
                       int *             outeriter,
                       int *             inneriter,
					   float *           work)
{
	vector float ix1,iy1,iz1,ix2,iy2,iz2,ix3,iy3,iz3;
	vector float jx1,jy1,jz1,jx2,jy2,jz2,jx3,jy3,jz3;

	vector float dx11,dy11,dz11,dx12,dy12,dz12,dx13,dy13,dz13;
	vector float dx21,dy21,dz21,dx22,dy22,dz22,dx23,dy23,dz23;
	vector float dx31,dy31,dz31,dx32,dy32,dz32,dx33,dy33,dz33;

	vector float rsq11,rsq12,rsq13,rsq21,rsq22,rsq23,rsq31,rsq32,rsq33;
	vector float rinv11,rinv12,rinv13,rinv21,rinv22,rinv23;
	vector float rinv31,rinv32,rinv33;

	vector float vfacel,nul;
	vector float vctot,qqMM,qqMH,qqHH,qM,qH,qqMMt,qqMHt,qqHHt;

	int n,k,ii,is3,ii3,nj0,nj1;
	int jnra,jnrb,jnrc,jnrd;
	int j3a,j3b,j3c,j3d;
	int nri, ntype, nouter, ninner;
#ifdef GMX_THREAD_SHM_FDECOMP
	int nn0, nn1;
#endif

    nouter   = 0;
    ninner   = 0;
    nri      = *p_nri;
    ntype    = *p_ntype;
	nul=vec_zero();
	vfacel=load_float_and_splat(p_facel);
	qH        = load_float_and_splat(charge+iinr[0]+1);
	qM        = load_float_and_splat(charge+iinr[0]+3);
	qqMM      = vec_madd(qM,qM,nul);
	qqMH      = vec_madd(qM,qH,nul);
	qqHH      = vec_madd(qH,qH,nul);
	qqMM      = vec_madd(qqMM,vfacel,nul);
	qqMH      = vec_madd(qqMH,vfacel,nul);
	qqHH      = vec_madd(qqHH,vfacel,nul);

#ifdef GMX_THREAD_SHM_FDECOMP
    nthreads = *p_nthreads;
	do {
		tMPI_Thread_mutex_lock((tMPI_Thread_mutex_t *)mtx);
		nn0              = *count;
		nn1              = nn0+(nri-nn0)/(2*nthreads)+3;
		*count           = nn1;
		tMPI_Thread_mutex_unlock((tMPI_Thread_mutex_t *)mtx);
		if(nn1>nri) nn1=nri;
		for(n=nn0; (n<nn1); n++) {
#if 0
		} /* maintain correct indentation even with conditional left braces */
#endif
#else /* without tMPI_Threads */
		for(n=0;n<nri;n++) {
#endif  
			is3        = 3*shift[n];
			ii         = iinr[n];
			ii3        = 3*ii;
			load_1_3atoms_shift_and_splat(pos+ii3+3,shiftvec+is3,
										  &ix1,&iy1,&iz1,
										  &ix2,&iy2,&iz2,&ix3,&iy3,&iz3);
			vctot      = nul;
			nj0        = jindex[n];
			nj1        = jindex[n+1];

			for(k=nj0; k<(nj1-3); k+=4) {
				jnra            = jjnr[k];
				jnrb            = jjnr[k+1];
				jnrc            = jjnr[k+2];
				jnrd            = jjnr[k+3];
				j3a             = 3*jnra;
				j3b             = 3*jnrb;
				j3c             = 3*jnrc;
				j3d             = 3*jnrd;
				load_4_3atoms(pos+j3a+3,pos+j3b+3,pos+j3c+3,pos+j3d+3,
							  &jx1,&jy1,&jz1,&jx2,&jy2,&jz2,&jx3,&jy3,&jz3);

				dx11            = vec_sub(ix1,jx1);
				dx12            = vec_sub(ix1,jx2);
				dx13            = vec_sub(ix1,jx3);
				dy11            = vec_sub(iy1,jy1);
				dy12            = vec_sub(iy1,jy2);
				dy13            = vec_sub(iy1,jy3);
				dz11            = vec_sub(iz1,jz1);
				dz12            = vec_sub(iz1,jz2);
				dz13            = vec_sub(iz1,jz3);
				dx21            = vec_sub(ix2,jx1);
				dx22            = vec_sub(ix2,jx2);
				dx23            = vec_sub(ix2,jx3);
				dy21            = vec_sub(iy2,jy1);
				dy22            = vec_sub(iy2,jy2);
				dy23            = vec_sub(iy2,jy3);
				dz21            = vec_sub(iz2,jz1);
				dz22            = vec_sub(iz2,jz2);
				dz23            = vec_sub(iz2,jz3);
				dx31            = vec_sub(ix3,jx1);
				dx32            = vec_sub(ix3,jx2);
				dx33            = vec_sub(ix3,jx3);
				dy31            = vec_sub(iy3,jy1);
				dy32            = vec_sub(iy3,jy2);
				dy33            = vec_sub(iy3,jy3);
				dz31            = vec_sub(iz3,jz1);
				dz32            = vec_sub(iz3,jz2);
				dz33            = vec_sub(iz3,jz3);

				rsq11           = vec_madd(dx11,dx11,nul);
				rsq12           = vec_madd(dx12,dx12,nul);
				rsq13           = vec_madd(dx13,dx13,nul);
				rsq21           = vec_madd(dx21,dx21,nul);
				rsq22           = vec_madd(dx22,dx22,nul);
				rsq23           = vec_madd(dx23,dx23,nul);
				rsq31           = vec_madd(dx31,dx31,nul);
				rsq32           = vec_madd(dx32,dx32,nul);
				rsq33           = vec_madd(dx33,dx33,nul);
				rsq11           = vec_madd(dy11,dy11,rsq11);
				rsq12           = vec_madd(dy12,dy12,rsq12);
				rsq13           = vec_madd(dy13,dy13,rsq13);
				rsq21           = vec_madd(dy21,dy21,rsq21);
				rsq22           = vec_madd(dy22,dy22,rsq22);
				rsq23           = vec_madd(dy23,dy23,rsq23);
				rsq31           = vec_madd(dy31,dy31,rsq31);
				rsq32           = vec_madd(dy32,dy32,rsq32);
				rsq33           = vec_madd(dy33,dy33,rsq33);
				rsq11           = vec_madd(dz11,dz11,rsq11);
				rsq12           = vec_madd(dz12,dz12,rsq12);
				rsq13           = vec_madd(dz13,dz13,rsq13);
				rsq21           = vec_madd(dz21,dz21,rsq21);
				rsq22           = vec_madd(dz22,dz22,rsq22);
				rsq23           = vec_madd(dz23,dz23,rsq23);
				rsq31           = vec_madd(dz31,dz31,rsq31);
				rsq32           = vec_madd(dz32,dz32,rsq32);
				rsq33           = vec_madd(dz33,dz33,rsq33);

				do_9_invsqrt(rsq11,rsq12,rsq13,
							 rsq21,rsq22,rsq23,
							 rsq31,rsq32,rsq33,
							 &rinv11,&rinv12,&rinv13,
							 &rinv21,&rinv22,&rinv23,
							 &rinv31,&rinv32,&rinv33);

				vctot           = vec_madd(rinv11,qqHH,vctot);
				vctot           = vec_madd(rinv12,qqHH,vctot);
				vctot           = vec_madd(rinv13,qqMH,vctot);
				vctot           = vec_madd(rinv21,qqHH,vctot);
				vctot           = vec_madd(rinv22,qqHH,vctot);
				vctot           = vec_madd(rinv23,qqMH,vctot);
				vctot           = vec_madd(rinv31,qqMH,vctot);
				vctot           = vec_madd(rinv32,qqMH,vctot);
				vctot           = vec_madd(rinv33,qqMM,vctot);
			} 
			if(k<(nj1-2)) {
				jnra            = jjnr[k];
				jnrb            = jjnr[k+1];
				jnrc            = jjnr[k+2];
				j3a             = 3*jnra;
				j3b             = 3*jnrb;
				j3c             = 3*jnrc;
				load_3_3atoms(pos+j3a+3,pos+j3b+3,pos+j3c+3,
							  &jx1,&jy1,&jz1,&jx2,&jy2,&jz2,&jx3,&jy3,&jz3);
				qqMMt           = vec_sld(qqMM,nul,4);
				qqMHt           = vec_sld(qqMH,nul,4);
				qqHHt           = vec_sld(qqHH,nul,4);

				dx11            = vec_sub(ix1,jx1);
				dx12            = vec_sub(ix1,jx2);
				dx13            = vec_sub(ix1,jx3);
				dy11            = vec_sub(iy1,jy1);
				dy12            = vec_sub(iy1,jy2);
				dy13            = vec_sub(iy1,jy3);
				dz11            = vec_sub(iz1,jz1);
				dz12            = vec_sub(iz1,jz2);
				dz13            = vec_sub(iz1,jz3);
				dx21            = vec_sub(ix2,jx1);
				dx22            = vec_sub(ix2,jx2);
				dx23            = vec_sub(ix2,jx3);
				dy21            = vec_sub(iy2,jy1);
				dy22            = vec_sub(iy2,jy2);
				dy23            = vec_sub(iy2,jy3);
				dz21            = vec_sub(iz2,jz1);
				dz22            = vec_sub(iz2,jz2);
				dz23            = vec_sub(iz2,jz3);
				dx31            = vec_sub(ix3,jx1);
				dx32            = vec_sub(ix3,jx2);
				dx33            = vec_sub(ix3,jx3);
				dy31            = vec_sub(iy3,jy1);
				dy32            = vec_sub(iy3,jy2);
				dy33            = vec_sub(iy3,jy3);
				dz31            = vec_sub(iz3,jz1);
				dz32            = vec_sub(iz3,jz2);
				dz33            = vec_sub(iz3,jz3);

				rsq11           = vec_madd(dx11,dx11,nul);
				rsq12           = vec_madd(dx12,dx12,nul);
				rsq13           = vec_madd(dx13,dx13,nul);
				rsq21           = vec_madd(dx21,dx21,nul);
				rsq22           = vec_madd(dx22,dx22,nul);
				rsq23           = vec_madd(dx23,dx23,nul);
				rsq31           = vec_madd(dx31,dx31,nul);
				rsq32           = vec_madd(dx32,dx32,nul);
				rsq33           = vec_madd(dx33,dx33,nul);
				rsq11           = vec_madd(dy11,dy11,rsq11);
				rsq12           = vec_madd(dy12,dy12,rsq12);
				rsq13           = vec_madd(dy13,dy13,rsq13);
				rsq21           = vec_madd(dy21,dy21,rsq21);
				rsq22           = vec_madd(dy22,dy22,rsq22);
				rsq23           = vec_madd(dy23,dy23,rsq23);
				rsq31           = vec_madd(dy31,dy31,rsq31);
				rsq32           = vec_madd(dy32,dy32,rsq32);
				rsq33           = vec_madd(dy33,dy33,rsq33);
				rsq11           = vec_madd(dz11,dz11,rsq11);
				rsq12           = vec_madd(dz12,dz12,rsq12);
				rsq13           = vec_madd(dz13,dz13,rsq13);
				rsq21           = vec_madd(dz21,dz21,rsq21);
				rsq22           = vec_madd(dz22,dz22,rsq22);
				rsq23           = vec_madd(dz23,dz23,rsq23);
				rsq31           = vec_madd(dz31,dz31,rsq31);
				rsq32           = vec_madd(dz32,dz32,rsq32);
				rsq33           = vec_madd(dz33,dz33,rsq33);

				do_9_invsqrt(rsq11,rsq12,rsq13,
							 rsq21,rsq22,rsq23,
							 rsq31,rsq32,rsq33,
							 &rinv11,&rinv12,&rinv13,
							 &rinv21,&rinv22,&rinv23,
							 &rinv31,&rinv32,&rinv33);
      
				zero_highest_element_in_9_vectors(&rinv11,&rinv12,&rinv13,
												  &rinv21,&rinv22,&rinv23,
												  &rinv31,&rinv32,&rinv33);

				vctot           = vec_madd(rinv11,qqHHt,vctot);
				vctot           = vec_madd(rinv12,qqHHt,vctot);
				vctot           = vec_madd(rinv13,qqMHt,vctot);
				vctot           = vec_madd(rinv21,qqHHt,vctot);
				vctot           = vec_madd(rinv22,qqHHt,vctot);
				vctot           = vec_madd(rinv23,qqMHt,vctot);
				vctot           = vec_madd(rinv31,qqMHt,vctot);
				vctot           = vec_madd(rinv32,qqMHt,vctot);
				vctot           = vec_madd(rinv33,qqMMt,vctot);
			} else if(k<(nj1-1)) {
				jnra            = jjnr[k];
				jnrb            = jjnr[k+1];
				j3a             = 3*jnra;
				j3b             = 3*jnrb;
				load_2_3atoms(pos+j3a+3,pos+j3b+3,
							  &jx1,&jy1,&jz1,&jx2,&jy2,&jz2,&jx3,&jy3,&jz3);
				qqMMt           = vec_sld(qqMM,nul,8);
				qqMHt           = vec_sld(qqMH,nul,8);
				qqHHt           = vec_sld(qqHH,nul,8);

				dx11            = vec_sub(ix1,jx1);
				dx12            = vec_sub(ix1,jx2);
				dx13            = vec_sub(ix1,jx3);
				dy11            = vec_sub(iy1,jy1);
				dy12            = vec_sub(iy1,jy2);
				dy13            = vec_sub(iy1,jy3);
				dz11            = vec_sub(iz1,jz1);
				dz12            = vec_sub(iz1,jz2);
				dz13            = vec_sub(iz1,jz3);
				dx21            = vec_sub(ix2,jx1);
				dx22            = vec_sub(ix2,jx2);
				dx23            = vec_sub(ix2,jx3);
				dy21            = vec_sub(iy2,jy1);
				dy22            = vec_sub(iy2,jy2);
				dy23            = vec_sub(iy2,jy3);
				dz21            = vec_sub(iz2,jz1);
				dz22            = vec_sub(iz2,jz2);
				dz23            = vec_sub(iz2,jz3);
				dx31            = vec_sub(ix3,jx1);
				dx32            = vec_sub(ix3,jx2);
				dx33            = vec_sub(ix3,jx3);
				dy31            = vec_sub(iy3,jy1);
				dy32            = vec_sub(iy3,jy2);
				dy33            = vec_sub(iy3,jy3);
				dz31            = vec_sub(iz3,jz1);
				dz32            = vec_sub(iz3,jz2);
				dz33            = vec_sub(iz3,jz3);

				rsq11           = vec_madd(dx11,dx11,nul);
				rsq12           = vec_madd(dx12,dx12,nul);
				rsq13           = vec_madd(dx13,dx13,nul);
				rsq21           = vec_madd(dx21,dx21,nul);
				rsq22           = vec_madd(dx22,dx22,nul);
				rsq23           = vec_madd(dx23,dx23,nul);
				rsq31           = vec_madd(dx31,dx31,nul);
				rsq32           = vec_madd(dx32,dx32,nul);
				rsq33           = vec_madd(dx33,dx33,nul);
				rsq11           = vec_madd(dy11,dy11,rsq11);
				rsq12           = vec_madd(dy12,dy12,rsq12);
				rsq13           = vec_madd(dy13,dy13,rsq13);
				rsq21           = vec_madd(dy21,dy21,rsq21);
				rsq22           = vec_madd(dy22,dy22,rsq22);
				rsq23           = vec_madd(dy23,dy23,rsq23);
				rsq31           = vec_madd(dy31,dy31,rsq31);
				rsq32           = vec_madd(dy32,dy32,rsq32);
				rsq33           = vec_madd(dy33,dy33,rsq33);
				rsq11           = vec_madd(dz11,dz11,rsq11);
				rsq12           = vec_madd(dz12,dz12,rsq12);
				rsq13           = vec_madd(dz13,dz13,rsq13);
				rsq21           = vec_madd(dz21,dz21,rsq21);
				rsq22           = vec_madd(dz22,dz22,rsq22);
				rsq23           = vec_madd(dz23,dz23,rsq23);
				rsq31           = vec_madd(dz31,dz31,rsq31);
				rsq32           = vec_madd(dz32,dz32,rsq32);
				rsq33           = vec_madd(dz33,dz33,rsq33);

				do_9_invsqrt(rsq11,rsq12,rsq13,
							 rsq21,rsq22,rsq23,
							 rsq31,rsq32,rsq33,
							 &rinv11,&rinv12,&rinv13,
							 &rinv21,&rinv22,&rinv23,
							 &rinv31,&rinv32,&rinv33);
      
				zero_highest_2_elements_in_9_vectors(&rinv11,&rinv12,&rinv13,
													 &rinv21,&rinv22,&rinv23,
													 &rinv31,&rinv32,&rinv33);

				vctot           = vec_madd(rinv11,qqHHt,vctot);
				vctot           = vec_madd(rinv12,qqHHt,vctot);
				vctot           = vec_madd(rinv13,qqMHt,vctot);
				vctot           = vec_madd(rinv21,qqHHt,vctot);
				vctot           = vec_madd(rinv22,qqHHt,vctot);
				vctot           = vec_madd(rinv23,qqMHt,vctot);
				vctot           = vec_madd(rinv31,qqMHt,vctot);
				vctot           = vec_madd(rinv32,qqMHt,vctot);
				vctot           = vec_madd(rinv33,qqMMt,vctot);
			} else if(k<nj1) {
				jnra            = jjnr[k];
				j3a             = 3*jnra;
				load_1_3atoms(pos+j3a+3,
							  &jx1,&jy1,&jz1,&jx2,&jy2,&jz2,&jx3,&jy3,&jz3);
				qqMMt           = vec_sld(qqMM,nul,12);
				qqMHt           = vec_sld(qqMH,nul,12);
				qqHHt           = vec_sld(qqHH,nul,12);

				dx11            = vec_sub(ix1,jx1);
				dx12            = vec_sub(ix1,jx2);
				dx13            = vec_sub(ix1,jx3);
				dy11            = vec_sub(iy1,jy1);
				dy12            = vec_sub(iy1,jy2);
				dy13            = vec_sub(iy1,jy3);
				dz11            = vec_sub(iz1,jz1);
				dz12            = vec_sub(iz1,jz2);
				dz13            = vec_sub(iz1,jz3);
				dx21            = vec_sub(ix2,jx1);
				dx22            = vec_sub(ix2,jx2);
				dx23            = vec_sub(ix2,jx3);
				dy21            = vec_sub(iy2,jy1);
				dy22            = vec_sub(iy2,jy2);
				dy23            = vec_sub(iy2,jy3);
				dz21            = vec_sub(iz2,jz1);
				dz22            = vec_sub(iz2,jz2);
				dz23            = vec_sub(iz2,jz3);
				dx31            = vec_sub(ix3,jx1);
				dx32            = vec_sub(ix3,jx2);
				dx33            = vec_sub(ix3,jx3);
				dy31            = vec_sub(iy3,jy1);
				dy32            = vec_sub(iy3,jy2);
				dy33            = vec_sub(iy3,jy3);
				dz31            = vec_sub(iz3,jz1);
				dz32            = vec_sub(iz3,jz2);
				dz33            = vec_sub(iz3,jz3);

				rsq11           = vec_madd(dx11,dx11,nul);
				rsq12           = vec_madd(dx12,dx12,nul);
				rsq13           = vec_madd(dx13,dx13,nul);
				rsq21           = vec_madd(dx21,dx21,nul);
				rsq22           = vec_madd(dx22,dx22,nul);
				rsq23           = vec_madd(dx23,dx23,nul);
				rsq31           = vec_madd(dx31,dx31,nul);
				rsq32           = vec_madd(dx32,dx32,nul);
				rsq33           = vec_madd(dx33,dx33,nul);
				rsq11           = vec_madd(dy11,dy11,rsq11);
				rsq12           = vec_madd(dy12,dy12,rsq12);
				rsq13           = vec_madd(dy13,dy13,rsq13);
				rsq21           = vec_madd(dy21,dy21,rsq21);
				rsq22           = vec_madd(dy22,dy22,rsq22);
				rsq23           = vec_madd(dy23,dy23,rsq23);
				rsq31           = vec_madd(dy31,dy31,rsq31);
				rsq32           = vec_madd(dy32,dy32,rsq32);
				rsq33           = vec_madd(dy33,dy33,rsq33);
				rsq11           = vec_madd(dz11,dz11,rsq11);
				rsq12           = vec_madd(dz12,dz12,rsq12);
				rsq13           = vec_madd(dz13,dz13,rsq13);
				rsq21           = vec_madd(dz21,dz21,rsq21);
				rsq22           = vec_madd(dz22,dz22,rsq22);
				rsq23           = vec_madd(dz23,dz23,rsq23);
				rsq31           = vec_madd(dz31,dz31,rsq31);
				rsq32           = vec_madd(dz32,dz32,rsq32);
				rsq33           = vec_madd(dz33,dz33,rsq33);

				do_9_invsqrt(rsq11,rsq12,rsq13,
							 rsq21,rsq22,rsq23,
							 rsq31,rsq32,rsq33,
							 &rinv11,&rinv12,&rinv13,
							 &rinv21,&rinv22,&rinv23,
							 &rinv31,&rinv32,&rinv33);
      
				zero_highest_3_elements_in_9_vectors(&rinv11,&rinv12,&rinv13,
													 &rinv21,&rinv22,&rinv23,
													 &rinv31,&rinv32,&rinv33);

				vctot           = vec_madd(rinv11,qqHHt,vctot);
				vctot           = vec_madd(rinv12,qqHHt,vctot);
				vctot           = vec_madd(rinv13,qqMHt,vctot);
				vctot           = vec_madd(rinv21,qqHHt,vctot);
				vctot           = vec_madd(rinv22,qqHHt,vctot);
				vctot           = vec_madd(rinv23,qqMHt,vctot);
				vctot           = vec_madd(rinv31,qqMHt,vctot);
				vctot           = vec_madd(rinv32,qqMHt,vctot);
				vctot           = vec_madd(rinv33,qqMMt,vctot);
			}
			/* update outer data */
			add_vector_to_float(Vc+gid[n],vctot);
			ninner += nj1 - nj0;
		}
#ifdef GMX_THREAD_SHM_FDECOMP
		nouter += nn1 - nn0;
	} while (nn1<nri);
#else
	nouter = nri;
#endif
	*outeriter = nouter;
	*inneriter = ninner;
}



