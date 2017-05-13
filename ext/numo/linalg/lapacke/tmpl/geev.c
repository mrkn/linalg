/*
<%
 aout = [         "{cT,1,shape}",
   !is_complex && "{cT,1,shape}",
                  "{cT,2,shape},{cT,2,shape},{cInt,0}"
 ].select{|x| x}.join(",")

 func_args = [
   "g->order, g->jobvl, g->jobvr, n, a, lda",
   is_complex ? "w" : "wr, wi",
   "vl, ldvl, vr, ldvr"
 ].join(",")

 tp = "Numo::"+class_name
 return_type = ([tp]*(is_complex ? 3 : 4) + ["Integer"]).join(", ")
 return_name = (is_complex ? "w,":"wr, wi,") + " vl, vr, info"
%>
*/
#define args_t <%=func_name%>_args_t
#define func_p <%=func_name%>_p

typedef struct {
    int order;
    char jobvl, jobvr;
} args_t;

static <%=func_name%>_t func_p = 0;

static void
<%=c_iter%>(na_loop_t * const lp)
{
    dtype *a, *vl, *vr;
#if IS_COMPLEX
    dtype *w;
#else
    dtype *wr, *wi;
#endif
    int   *info;
    int    n, lda, ldvl, ldvr;
    args_t *g;

    a = (dtype*)NDL_PTR(lp,0);
#if IS_COMPLEX
#define N 0
    w = (dtype*)NDL_PTR(lp,1);
#else
#define N 1
    wr = (dtype*)NDL_PTR(lp,1);
    wi = (dtype*)NDL_PTR(lp,2);
#endif
    vl = (dtype*)NDL_PTR(lp,N+2);
    vr = (dtype*)NDL_PTR(lp,N+3);
    info = (int*)NDL_PTR(lp,N+4);
    g = (args_t*)(lp->opt_ptr);

    n = lp->args[0].shape[1];
    lda = lp->args[0].iter[0].step / sizeof(dtype);
    ldvl = lp->args[N+2].iter[0].step / sizeof(dtype);
    if (ldvl == 0) { ldvl = n; } // jobvt == 'N'
    ldvr = lp->args[N+3].iter[0].step / sizeof(dtype);
    if (ldvr == 0) { ldvr = n; } // jobvt == 'N'

    //printf("order=%d jobvl=%c jobvr=%c n=%d lda=%d ldvl=%d ldvr=%d\n",g->order,g->jobvl, g->jobvr, n, lda,ldvl,ldvr);

    *info = (*func_p)( <%=func_args%> );
    CHECK_ERROR(*info);
}

/*
  @overload <%=name%>(a, b [,jobvl:'v', jobvr:'v'] )
  @param [Numo::<%=class_name%>] a >=2-dimentional NArray.
  @param [Numo::<%=class_name%>] b >=2-dimentional NArray.
  @param [String,Symbol] jobvl
    jobvl='N':  do not compute the left generalized eigenvectors;
    jobvl='V':  compute the left generalized eigenvectors.
  @param [String,Symbol] jobvr
    jobvr='N':  do not compute the left generalized eigenvectors;
    jobvr='V':  compute the left generalized eigenvectors.
  @return [[<%=return_type%>]] array of [<%=return_name%>]

  <%=description%>
*/
static VALUE
<%=c_func(-1)%>(int argc, VALUE const argv[], VALUE UNUSED(mod))
{
    VALUE a, ans;
    int   m, n;
    narray_t *na1;
    size_t shape[2];
    ndfunc_arg_in_t ain[1] = {{OVERWRITE,2}};
    ndfunc_arg_out_t aout[N+4] = {<%=aout%>};
    ndfunc_t ndf = {&<%=c_iter%>, NO_LOOP|NDF_EXTRACT, 1, N+4, ain, aout};

    args_t g = {0,0,0};
    VALUE opts[3] = {Qundef,Qundef,Qundef};
    VALUE kw_hash = Qnil;
    ID kw_table[3] = {id_order,id_jobvl,id_jobvr};

    CHECK_FUNC(func_p,"<%=func_name%>");

    rb_scan_args(argc, argv, "1:", &a, &kw_hash);
    rb_get_kwargs(kw_hash, kw_table, 0, 3, opts);
    g.order = option_order(opts[0]);
    g.jobvl = option_job(opts[1],'V');
    g.jobvr = option_job(opts[2],'V');

    COPY_OR_CAST_TO(a,cT);
    GetNArray(a, na1);
    CHECK_DIM_GE(na1, 2);
    m = ROW_SIZE(na1);
    n = COL_SIZE(na1);
    if (m != n) {
        rb_raise(nary_eShapeError,"matrix must be square");
    }
    shape[0] = shape[1] = n;
    if (g.jobvl=='N') { aout[N+1].dim = 0; }
    if (g.jobvr=='N') { aout[N+2].dim = 0; }

    ans = na_ndloop3(&ndf, &g, 1, a);

    if (aout[N+2].dim == 0) { RARRAY_ASET(ans,N+2,Qnil); }
    if (aout[N+1].dim == 0) { RARRAY_ASET(ans,N+1,Qnil); }
    return ans;
}

#undef N
#undef args_t
#undef func_p
