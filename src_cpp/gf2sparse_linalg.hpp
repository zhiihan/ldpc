#ifndef GF2LINALG_H
#define GF2LINALG_H

#include <iostream>
#include <vector>
#include <memory>
#include <iterator>
#include <algorithm>
#include <limits>
#include "gf2sparse.hpp"
#include "sparse_matrix_util.hpp"

// using namespace std; 
// using namespace gf2sparse;

namespace gf2sparse_linalg{

std::vector<int> NULL_INT_VECTOR = {};

template <class ENTRY_OBJ>
class RowReduce{

    public:
    
        gf2sparse::GF2Sparse<ENTRY_OBJ>& A;
        gf2sparse::GF2Sparse<> L;
        gf2sparse::GF2Sparse<> U;
        gf2sparse::GF2Sparse<> P;
        std::vector<int> rows;
        std::vector<int> cols;
        std::vector<int> inv_rows;
        std::vector<int> inv_cols;
        std::vector<bool> pivots;
        std::vector<uint8_t> x;
        std::vector<uint8_t> b;
        bool FULL_REDUCE;
        bool LU_ALLOCATED;
        bool LOWER_TRIANGULAR;
        int rank;

        RowReduce() = default;

        RowReduce(gf2sparse::GF2Sparse<ENTRY_OBJ>& A): A(A){

            this->pivots.resize(this->A.n,false);
            this->cols.resize(this->A.n);
            this->rows.resize(this->A.m);
            this->inv_cols.resize(this->A.n);
            this->inv_rows.resize(this->A.m);
            this->x.resize(this->A.n);
            this->b.resize(this->A.m);
            this->LU_ALLOCATED = false;
            this->LOWER_TRIANGULAR = false;

        }

        void initialise(){
            
            this->pivots.resize(this->A.n,false);
            this->cols.resize(this->A.n);
            this->rows.resize(this->A.m);
            this->inv_cols.resize(this->A.n);
            this->inv_rows.resize(this->A.m);
            this->x.resize(this->A.n);
            this->b.resize(this->A.m);
            this->LU_ALLOCATED = false;
            this->LOWER_TRIANGULAR = false;
            
        }

        ~RowReduce() = default;

        void initiliase_LU(){

            this->U.allocate(this->A.m,this->A.n);
            this->L.allocate(this->A.m,this->A.m);

            for(int i = 0; i<this->A.m; i++){
                for(auto& e: this->A.iterate_row(i)){
                    this->U.insert_entry(e.row_index, e.col_index);
                }
                if(!this->LOWER_TRIANGULAR) this->L.insert_entry(i,i);
            }

            this->LU_ALLOCATED = true;

        }
        
        void set_column_row_orderings(std::vector<int>& cols = NULL_INT_VECTOR, std::vector<int>& rows = NULL_INT_VECTOR){
            
            if(cols==NULL_INT_VECTOR){
                for(int i = 0; i<this->A.n; i++){
                    this->cols[i] = i;
                    this->inv_cols[this->cols[i]] = i;
                }
            }
            else{
                if(cols.size()!=this->A.n) throw std::invalid_argument("Input parameter `cols`\
                describing the row ordering is of the incorrect length");
                // this->cols=cols;
                for(int i = 0; i<this->A.n; i++){
                    this->cols[i] = cols[i];
                    inv_cols[cols[i]] = i;
                }
            }

            if(rows==NULL_INT_VECTOR){
                for(int i = 0; i<this->A.m; i++){
                    this->rows[i] = i;
                    this->inv_rows[this->rows[i]] = i;
                }
            }
            else{
                if(rows.size()!=this->A.m) throw std::invalid_argument("Input parameter `rows`\
                describing the row ordering is of the incorrect length");
                // this->rows=rows;
                for(int i = 0; i<this->A.m; i++){
                    this->rows[i] = rows[i];
                    this->inv_rows[rows[i]] = i;
                }
            }

        }




        int rref(bool full_reduce = false, bool lower_triangular = false, std::vector<int>& cols = NULL_INT_VECTOR, std::vector<int>& rows = NULL_INT_VECTOR){

            this->LOWER_TRIANGULAR = lower_triangular;
            this->FULL_REDUCE = full_reduce;
            this->set_column_row_orderings(cols,rows);
            this->initiliase_LU();
            int max_rank = std::min(this->U.m,this->U.n);
            this->rank = 0;
            std::fill(this->pivots.begin(),this->pivots.end(), false);

            for(int j = 0; j<this->U.n; j++){

                // cout<<"Column "<<j<<endl;
                // // cout<<U.sparsity()<<endl;
                // cout<<"U entry count: "<<U.entry_count()<<"; Sparsity: "<<U.sparsity()<<endl;
                // cout<<"L entry count: "<<L.entry_count()<<"; Sparsity: "<<L.sparsity()<<endl;
                // cout<<endl;

                int pivot_index = this->cols[j];
       
                if(this->rank == max_rank) break;


                bool PIVOT_FOUND = false;
                int max_row_weight = std::numeric_limits<int>::max();
                int swap_index;
                for(auto& e: this->U.iterate_column(pivot_index)){
                    int row_index = e.row_index;

                    if(row_index<this->rank) continue;

                    //finds the total row weight across both L and U for the given row
                    int row_weight = this->U.get_row_degree(row_index) + this->L.get_row_degree(row_index);

                    if(row_weight<max_row_weight){
                        swap_index = e.row_index;
                        max_row_weight = row_weight;
                    }
                    PIVOT_FOUND=true;
                    this->pivots[j] = true;
                }
                
                if(!PIVOT_FOUND) continue;

                if(swap_index!=this->rank){
                    U.swap_rows(swap_index,this->rank);
                    L.swap_rows(swap_index,this->rank);
                    auto temp1 = this->rows[swap_index];
                    auto temp2 = this->rows[this->rank];
                    this->rows[swap_index] = temp2;
                    this->rows[this->rank] = temp1;
                }

                if(this->LOWER_TRIANGULAR) this->L.insert_entry(this->rank,this->rank);

                std::vector<int> add_rows;
                for(auto& e: this->U.iterate_column(pivot_index)){
                    int row_index = e.row_index;
                    if(row_index>this->rank || row_index<this->rank && this->FULL_REDUCE==true){
                        add_rows.push_back(row_index);
                    }
                }

                for(int row: add_rows){
                    this->U.add_rows(row,this->rank);
                    if(this->LOWER_TRIANGULAR) this->L.insert_entry(row,this->rank);
                    else this->L.add_rows(row,this->rank);
                }

                this->rank++;  

            }

            int pivot_count = 0;
            int non_pivot_count = 0;
            auto orig_cols = this->cols;
            for(int i=0; i<this->U.n; i++){
                if(this->pivots[i]){
                    this->cols[pivot_count] = orig_cols[i];
                    this->inv_cols[this->cols[pivot_count]] = pivot_count;
                    pivot_count++;
                }
                else{
                    this->cols[this->rank + non_pivot_count] = orig_cols[i];
                    this->inv_cols[this->cols[this->rank + non_pivot_count]] = this->rank + non_pivot_count;
                    non_pivot_count++; 
                }
            }

            return this->rank;

        }

        void build_p_matrix(){

            if(!this->LU_ALLOCATED || !this->LOWER_TRIANGULAR){
                this->rref(false,true);
            }
            this->P.allocate(this->A.m,this->A.m);
            for(int i = 0; i<this->A.m; i++){
                this->P.insert_entry(this->rows[i],i);
            }

        }

        std::vector<uint8_t>& lu_solve(std::vector<uint8_t>& y){

            if(y.size()!=this->U.m) throw std::invalid_argument("Input parameter `y` is of the incorrect length for lu_solve.");

            /*
            Equation: Ax = y
 
            We use LU decomposition to arrange the above into the form:
            LU(Qx) = PAQ^T(Qx)=Py

            We can then solve for x using forward-backward substitution:
            1. Forward substitution: Solve Lb = Py for b
            2. Backward subsitution: Solve UQx = b for x
            */


            if(!this->LU_ALLOCATED || !this->LOWER_TRIANGULAR){
                this->rref(false,true);
            }

            std::fill(this->x.begin(),this->x.end(), 0);
            std::fill(this->b.begin(),this->b.end(), 0);

            // //Solves LUx=y
            // int row_sum;



            //First we solve Lb = y, where b = Ux
            //Solve Lb=y with forwared substitution
            for(int row_index=0;row_index<this->rank;row_index++){
                int row_sum=0;
                for(auto& e: this->L.iterate_row(row_index)){
                    row_sum ^= this->b[e.col_index];
                }
                this->b[row_index]=row_sum^y[this->rows[row_index]];
            }





            //Solve Ux = b with backwards substitution
            for(int row_index=(this->rank-1);row_index>=0;row_index--){
                int row_sum=0;
                for(auto& e: this->U.iterate_row(row_index)){
                    row_sum ^= this->x[e.col_index];
                }
                this->x[this->cols[row_index]] = row_sum ^ this->b[row_index];
            }

            return this->x;
       
        }


        auto rref_vrs(bool full_reduce = false, bool lower_triangular = false, std::vector<int>& cols = NULL_INT_VECTOR, std::vector<int>& rows = NULL_INT_VECTOR){

            if(lower_triangular) this->LOWER_TRIANGULAR = true;
            this->set_column_row_orderings(cols,rows);
            this->initiliase_LU();
            int max_rank = std::min(this->U.m,this->U.n);
            this->rank = 0;
            std::fill(this->pivots.begin(),this->pivots.end(), false);

            for(int j = 0; j<this->U.n; j++){

                int pivot_index = this->cols[j];
       
                if(this->rank == max_rank) break;


                bool PIVOT_FOUND = false;
                int max_row_weight = std::numeric_limits<int>::max();
                int swap_index;
                for(auto& e: this->U.iterate_column(pivot_index)){
                    // int row_index = e.row_index;

                    if(this->inv_rows[e.row_index]<this->rank) continue;

                    int row_weight = this->U.get_row_degree(e.row_index) + this->L.get_row_degree(e.row_index);
                    if(this->inv_rows[e.row_index] >= this->rank && row_weight<max_row_weight){
                        swap_index = this->inv_rows[e.row_index];
                        max_row_weight = row_weight;
                    }
                    PIVOT_FOUND=true;
                    this->pivots[j] = true;
                }
                
                if(!PIVOT_FOUND) continue;

                if(swap_index!=this->rank){
                    // cout<<"Swapping rows "<<swap_index<<" and "<<this->rank<<endl;
                    // U.swap_rows(swap_index,this->inv_rows[this->rank]);
                    // L.swap_rows(swap_index,this->inv_rows[this->rank]);
                    auto temp1 = this->rows[swap_index];
                    auto temp2 = this->rows[this->rank];
                    this->rows[this->rank] = temp1;
                    this->rows[swap_index] = temp2;
                    this->inv_rows[temp1] = this->rank;
                    this->inv_rows[temp2] = swap_index;
                  
                }

                if(this->LOWER_TRIANGULAR) this->L.insert_entry(this->rows[this->rank],this->rank);
                // cout<<"Lower triangular: "<<endl;;
                // print_sparse_matrix(*this->L);
                // cout<<endl;


                std::vector<int> add_rows;
                for(auto& e: this->U.iterate_column(pivot_index)){
                    // int row_index = e.row_index;
                    if(this->inv_rows[e.row_index]>this->rank || this->inv_rows[e.row_index]<this->rank && full_reduce==true){
                        add_rows.push_back(e.row_index);
                    }
                }

                for(int row: add_rows){
                    this->U.add_rows(row,this->rows[this->rank]);
                    if(lower_triangular) this->L.insert_entry(row,this->rank);
                    else this->L.add_rows(row,this->rows[this->rank]);
                    // cout<<"Adding row "<<row<<" to row "<<this->rows[this->rank]<<endl;
                    // print_sparse_matrix(*this->U);
                    // cout<<endl;
                }

                this->rank++;  

            }

            int pivot_count = 0;
            int non_pivot_count = 0;
            auto orig_cols = this->cols;
            for(int i=0; i<this->U.n; i++){
                if(this->pivots[i]){
                    this->cols[pivot_count] = orig_cols[i];
                    this->inv_cols[this->cols[pivot_count]] = pivot_count;
                    pivot_count++;
                }
                else{
                    this->cols[this->rank + non_pivot_count] = orig_cols[i];
                    this->inv_cols[this->cols[this->rank + non_pivot_count]] = this->rank + non_pivot_count;
                    non_pivot_count++; 
                }
            }

            return this->U;

        }

        std::vector<uint8_t>& lu_solve_vrs(std::vector<uint8_t>& y){

            if(y.size()!=this->U.m) throw std::invalid_argument("Input parameter `y` is of the incorrect length for lu_solve.");

            /*
            Equation: Ax = y
 
            We use LU decomposition to arrange the above into the form:
            LU(Qx) = PAQ^T(Qx)=Py
            We can then solve for x using forward-backward substitution:
            1. Forward substitution: Solve Lb = Py for b
            2. Backward subsitution: Solve UQx = b for x
            */


            if(!this->LU_ALLOCATED || !this->LOWER_TRIANGULAR){
                this->rref(false,true);
            }

            std::fill(this->x.begin(),this->x.end(), 0);
            std::fill(this->b.begin(),this->b.end(), 0);

            //Solves LUx=y
            int row_sum;



            //First we solve Lb = y, where b = Ux
            //Solve Lb=y with forwared substitution
            for(int row_index=0;row_index<this->rank;row_index++){
                row_sum=0;
                for(auto& e: L.iterate_row(this->rows[row_index])){
                    row_sum^=b[e.col_index];
                }
                b[row_index]=row_sum^y[this->rows[row_index]];
            }





            //Solve Ux = b with backwards substitution
            for(int row_index=(rank-1);row_index>=0;row_index--){
                row_sum=0;
                for(auto& e: U.iterate_row(this->rows[row_index])){
                    row_sum^=x[e.col_index];
                }
                x[this->cols[row_index]] = row_sum^b[row_index];
            }

            return x;
       
        }


};


template <class GF2MATRIX>
std::vector<std::vector<int>> kernel_adjacency_list(GF2MATRIX& mat){


    auto matT = mat.transpose();
    
    // cout<<"Transpose of input matrix: "<<endl;

    auto rr = RowReduce(matT);
    rr.rref(false,false);

    // cout<<"Rref done"<<endl;

    int rank = rr.rank;
    int n = mat.n;
    int k = n - rank;
    // auto ker = GF2MATRIX::New(k,n);

    std::vector<std::vector<int>> ker;
    ker.resize(k,std::vector<int>{});

    for(int i = rank; i<n; i++){
        for(auto& e: rr.L.iterate_row(i)){
            ker[i-rank].push_back(e.col_index);
        }
    }
    
    return ker;

}


template <class GF2MATRIX>
GF2MATRIX kernel(GF2MATRIX& mat){

    auto adj_list = kernel_adjacency_list(mat);
    auto ker = GF2MATRIX(adj_list.size(),mat.n);
    ker.csr_insert(adj_list);

    return ker;

}

//cython helper
sparse_matrix_base::CsrMatrix cy_kernel(gf2sparse::GF2Sparse<gf2sparse::GF2Entry>* mat){
    return kernel(*mat).to_csr_matrix();
}

template <class GF2MATRIX>
int rank(GF2MATRIX& mat){
    auto rr = RowReduce(mat);
    rr.rref(false,false);
    return  rr.rank;
}

template <class GF2MATRIX>
GF2MATRIX row_complement_basis(GF2MATRIX& mat){
    auto matT = mat.transpose();
    
    auto id_mat = GF2MATRIX(mat.m, mat.m, mat.m);
    for(int i = 0; i<mat.m; i++){
        id_mat.insert_entry(i,i);
    }

    auto mat_aug = gf2sparse::hstack(matT,id_mat);

    auto rr = RowReduce(mat_aug);
    rr.rref(false,false);


    std::vector<int> basis_rows;
    for(int  i = 0; i<rr.rank; i++){
        if(rr.cols[i] >= matT.n){
            basis_rows.push_back(rr.cols[i] - matT.n); 
        }
    }

    GF2MATRIX basis = GF2MATRIX(basis_rows.size(), mat.n);
    for(int i = 0; i<basis_rows.size(); i++){
        basis.insert_entry(i,basis_rows[i]);
    }

    return basis;


}





}//end namespace gf2sparse_linalg

// typedef gf2sparse_linalg::RowReduce<gf2sparse::gf2sparse::GF2Sparse<gf2sparse::GF2Entry>> cy_row_reduce;

#endif