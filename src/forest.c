#include "forest.h"

static double fv(const MilenaArray *a, size_t i) { return a->dtype == MILENA_DTYPE_INT64 ? (double)((const int64_t *)milena_array_const_data(a))[i] : ((const double *)milena_array_const_data(a))[i]; }
static int64_t majority(const MilenaArray *y, const size_t *rows, size_t n) {
    const int64_t *v = milena_array_const_data(y); int64_t best = v[rows[0]]; size_t bc = 0;
    for (size_t i=0;i<n;i++){size_t c=0;for(size_t j=0;j<n;j++)if(v[rows[j]]==v[rows[i]])c++;if(c>bc){bc=c;best=v[rows[i]];}}
    return best;
}
static size_t add_node(MilenaForestClassifier *f, MilenaForestNode node) {
    size_t i=f->node_count++; f->nodes[i]=node; return i;
}
static size_t build(MilenaForestClassifier *f,const MilenaArray *x,const MilenaArray *y,const size_t *rows,size_t n,size_t depth,size_t tree,MilenaError *e){
    (void)e; int64_t cls=majority(y,rows,n); const int64_t *yv=milena_array_const_data(y);
    bool pure=true;for(size_t i=1;i<n;i++)if(yv[rows[i]]!=yv[rows[0]]){pure=false;break;}
    size_t node=add_node(f,(MilenaForestNode){0,0,0,0,cls,true});
    if(depth>=f->max_depth||pure||n<2)return node;
    size_t feature=(depth+tree)%f->feature_count; double lo=fv(x,rows[0]*x->shape[1]+feature),hi=lo;
    for(size_t i=1;i<n;i++){double z=fv(x,rows[i]*x->shape[1]+feature);if(z<lo)lo=z;if(z>hi)hi=z;}
    if(lo==hi)return node; double cut=(lo+hi)/2.0;size_t nl=0,nr=0;for(size_t i=0;i<n;i++)(fv(x,rows[i]*x->shape[1]+feature)<=cut?nl:nr)++;
    if(!nl||!nr)return node;size_t *left=malloc(nl*sizeof(*left)),*right=malloc(nr*sizeof(*right));if(!left||!right){free(left);free(right);return node;}
    size_t il=0,ir=0;for(size_t i=0;i<n;i++){if(fv(x,rows[i]*x->shape[1]+feature)<=cut)left[il++]=rows[i];else right[ir++]=rows[i];}
    size_t a=build(f,x,y,left,nl,depth+1,tree,e),b=build(f,x,y,right,nr,depth+1,tree,e);free(left);free(right);
    f->nodes[node]=(MilenaForestNode){feature,cut,a,b,cls,false};return node;
}
void milena_forest_init(MilenaForestClassifier *f){if(f){memset(f,0,sizeof(*f));}}
void milena_forest_release(MilenaForestClassifier *f){if(f){free(f->roots);free(f->nodes);milena_forest_init(f);}}
MilenaStatus milena_forest_train_depth(MilenaForestClassifier *f,const MilenaArray *x,const MilenaArray *y,size_t trees,size_t depth,MilenaError *e){
 if(!f||!x||!y||!x->storage||!y->storage||x->ndim!=2||y->ndim!=1||x->shape[0]!=y->shape[0]||!x->shape[0]||!x->shape[1]||!trees||!depth||(x->dtype!=MILENA_DTYPE_INT64&&x->dtype!=MILENA_DTYPE_FLOAT64)||y->dtype!=MILENA_DTYPE_INT64){milena_error_set(e,MILENA_ERR_ARGUMENT,0,0,0,"Datos inválidos para bosque");return MILENA_ERR_ARGUMENT;}
 MilenaForestClassifier n={0};n.tree_count=trees;n.feature_count=x->shape[1];n.max_depth=depth;n.roots=calloc(trees,sizeof(*n.roots));n.nodes=calloc(trees*((size_t)1<<(depth+1)),sizeof(*n.nodes));if(!n.roots||!n.nodes){free(n.roots);free(n.nodes);return MILENA_ERR_MEMORY;}
 size_t *rows=malloc(x->shape[0]*sizeof(*rows));if(!rows){milena_forest_release(&n);return MILENA_ERR_MEMORY;}for(size_t i=0;i<x->shape[0];i++)rows[i]=i;for(size_t t=0;t<trees;t++)n.roots[t]=build(&n,x,y,rows,x->shape[0],0,t,e);free(rows);milena_forest_release(f);*f=n;return MILENA_OK;
}
MilenaStatus milena_forest_train(MilenaForestClassifier *f,const MilenaArray *x,const MilenaArray *y,size_t trees,MilenaError *e){return milena_forest_train_depth(f,x,y,trees,1,e);}
MilenaStatus milena_forest_predict(const MilenaForestClassifier *f,const MilenaArray *x,MilenaArray *out,MilenaError *e){
 if(!f||!f->roots||!x||x->ndim!=2||x->shape[1]!=f->feature_count){milena_error_set(e,MILENA_ERR_ARGUMENT,0,0,0,"Datos inválidos para predicción");return MILENA_ERR_ARGUMENT;}size_t sh[]={x->shape[0]};MilenaStatus s=milena_array_zeros(out,MILENA_DTYPE_INT64,1,sh,e);if(s!=MILENA_OK)return s;int64_t *p=milena_array_data(out);for(size_t r=0;r<x->shape[0];r++){int64_t *votes=calloc(f->tree_count,sizeof(*votes));for(size_t t=0;t<f->tree_count;t++){size_t n=f->roots[t];while(!f->nodes[n].leaf)n=fv(x,r*x->shape[1]+f->nodes[n].feature)<=f->nodes[n].threshold?f->nodes[n].left:f->nodes[n].right;votes[t]=f->nodes[n].prediction;}size_t best=0,bc=0;for(size_t i=0;i<f->tree_count;i++){size_t c=0;for(size_t j=0;j<f->tree_count;j++)if(votes[i]==votes[j])c++;if(c>bc){bc=c;best=i;}}p[r]=votes[best];free(votes);}return MILENA_OK;}
