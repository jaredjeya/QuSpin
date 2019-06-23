#ifndef _user_basis_core_H
#define _user_basis_core_H

#include <complex>
#include <vector>
#include <stdio.h>
#include "general_basis_core.h"
#include "numpy/ndarraytypes.h"
#include "benes_perm.h"
#include "openmp.h"

namespace basis_general {

template<class I>
struct op_results
{
	std::complex<double> m;
	I r;
	op_results(std::complex<double> _m,I _r): m(_m),r(_r)
	{}
};


template<class I,class P=signed char>
class user_basis_core : public general_basis_core<I,P>
{
	typedef I (*map_type)(I,int,P*,const I*);
	typedef I (*next_state_type)(I,I,I,I*);
	typedef int (*op_func_type)(op_results<I>*,char,int,int);
	typedef void (*count_particles_type)(I,int*);
	typedef bool (*check_state_nosymm_type)(I,I,I*);

	public:
		map_type * map_funcs;
		next_state_type next_state_func;
		op_func_type op_func;
		count_particles_type count_particles_func;
		check_state_nosymm_type pre_check_state;
		const int n_sectors;
		I *ns_args,*precs_args;
		const I * const *maps_args;


		user_basis_core(const int _N,const int _nt,
			void * _map_funcs, const int _pers[], const int _qs[], const I* const * _maps_args, 
			const int _n_sectors,size_t _next_state,I *_ns_args,size_t _pre_check_state,
			I* _precs_args,size_t _count_particles,size_t _op_func) : \
		general_basis_core<I,P>::general_basis_core(_N,_nt,NULL,_pers,_qs), n_sectors(_n_sectors)
		{
			map_funcs = (map_type*)_map_funcs;
			maps_args = _maps_args;
			next_state_func = (next_state_type)_next_state;
			count_particles_func = (count_particles_type)_count_particles;
			op_func = (op_func_type)_op_func;
			ns_args = _ns_args;
			pre_check_state = (check_state_nosymm_type)_pre_check_state;
			precs_args = _precs_args;


			for(int i=0;i<3;i++){
				std::cout << i << " , " << maps_args[i] << " , " << maps_args[i][0] << std::endl; 
			}
			std::cout << maps_args[0] << " , " << maps_args[1] << " , " << maps_args[2] << " , " << '\n' << std::endl; 

		}

		~user_basis_core() {}

		I map_state(I s,int n_map,P &phase){
			if(general_basis_core<I,P>::nt<=0){
				return s;
			}
			std::cout << "here" << " , " << maps_args[0] << " , " << maps_args[1] << " , " << maps_args[2] << " , " << std::endl;
			// std::cout << n_map << std::endl;
			// std::cout << maps_args[n_map] << " , " << maps_args[n_map][0] << std::endl;
			
			P temp_phase = 1;
			s = (*map_funcs[n_map])(s, general_basis_core<I,P>::N, &temp_phase, maps_args[n_map]);
			phase *= temp_phase;
			return s;

		}

		void map_state(I s[],npy_intp M,int n_map,P phase[]){
			if(general_basis_core<I,P>::nt<=0){
				return;
			}
			map_type func = map_funcs[n_map];
			const I * args = maps_args[n_map];
			#pragma omp for schedule(static)
			for(npy_intp i=0;i<M;i++){
				P temp_phase = 1;
				s[i] = (*func)(s[i], general_basis_core<I,P>::N, &temp_phase, args);
				phase[i] *= temp_phase;

			}
		}

		std::vector<int> count_particles(const I s){
			std::vector<int> v(n_sectors);
			(*count_particles_func)(s,&v[0]);
			return v;
		}

		I inline next_state_pcon(const I s,const I nns){
			return (*next_state_func)(s,nns,(I)general_basis_core<I,P>::N, ns_args);
		}

		double check_state(I s){

			bool ns_check=true;

			if(pre_check_state){
				ns_check = (*pre_check_state)(s,(I)general_basis_core<I,P>::N,precs_args);
			}			
			
			if(ns_check){
				return check_state_core_unrolled<I>(this,s,general_basis_core<I,P>::nt);
			}
			else{
				return std::numeric_limits<double>::quiet_NaN();
			}
		}

		int op(I &r,std::complex<double> &m,const int n_op,const char opstr[],const int indx[]){
			I s = r;
			op_results<I> res(m,r);
			for(int j=n_op-1;j>=0;j--){
				int err = (*op_func)(&res,opstr[j],indx[j],general_basis_core<I,P>::N);
				if(err!=0){
					return err;
				}
				if(std::abs(res.m)==0){
					res.r = s;
					break;
				}
			}
			m = res.m; r = res.r;
			return 0;
		}
};


}




#endif
