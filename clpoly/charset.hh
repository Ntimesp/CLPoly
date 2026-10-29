/**
 * @file charset.hh
 * @author ntimesp(nxp@mail.ustc.edu.cn) 李昊坤(ker@pm.me)  
 * @brief  charset
 * 
 * 
 */

#ifndef CLPOLY_CHARSET_HH
#define CLPOLY_CHARSET_HH
#include <clpoly/polynomial.hh>

namespace clpoly{
    // 变量与顺序来自各多项式的 lex_<custom_var_order> 比较器，无需另传 vars。
    // 例如比较器优先级为 {x1,x2,x3} 时，三角列按主变量 x3、x2、x1 排列。
    // 调用者须保证多项式已规范化、比较器一致且在整个调用期间有效。

    // 比较吴秩：先比较论文记号 cls（主变量在文献变量序中的编号），再比较主变量次数。
    // 非零常数的 cls 为 0；cls 和主变量次数均相同视为同秩。
    // 常数同秩且低于非常数；代码将零也归为常数，但 BasSet 输入必须排除零。
    template <class Tc>
    bool rankcompare
    (const polynomial_<Tc,lex_<custom_var_order>> &F,
    const polynomial_<Tc,lex_<custom_var_order>> &G)
    {
        assert(comp_consistent(F.comp(),G.comp()));
        //比较两个多项式秩的大小 rank(F)<rank(G)?
        if(is_number(G))    return false;
        else if(is_number(F))   return true;
        //都不是常数多项式
        // auto m=F.comp().comp.v_map;
        // auto x=get_last_var_deg(F);
        // auto y=get_last_var_deg(G);
        auto & comp=F.comp();
        auto x=get_first_var(F);
        auto y=get_first_var(G);
        
        if(comp(y,x)) return true;
        if(x!=y) return false;
        if(get_first_deg(F)<get_first_deg(G))    return true;
        return false;
    }

    // 按 G 的主变量判断 deg(F,mvar(G)) < deg(G,mvar(G))。
    // 前提：cls(G)>0，即 G 非常数；F 可以是零或非零常数，此时返回 true。
    template <class Tc>
    bool is_reduced
    (const polynomial_<Tc,lex_<custom_var_order>> &F,
    const polynomial_<Tc,lex_<custom_var_order>> &G)
    {
        assert(!is_number(G));
        return degree(F,get_first_var(G))<get_first_deg(G);
    }

    template <class comp>
    polynomial_<ZZ,comp> sqrfree(const polynomial_<ZZ,comp> & F)
    {
        //去掉多项式系数公因子，去平方
        polynomial_<ZZ,comp> G(F.comp_ptr());
        G=1;
        if(F.empty()) return F;
        auto lst=squarefreefactorize(F);
        
        for(auto i=lst.begin()+1;i<lst.end();i++)
            G=G*(*i).first;
        return G;
    }

    template <class comp> //重复功能 应该换成squarefreebasis
    std::vector<polynomial_<ZZ,comp>> sqrfree(const std::vector<polynomial_<ZZ,comp>> & F)
    {
        //去掉多项式组中每个多项式系数公因子，去平方
        std::vector<polynomial_<ZZ,comp>> G;
        for(auto i:F)
            G.push_back(sqrfree(i));
        return G;
    }

    // 内部基列算法，由 charset 调用；王东明《计算机代数》书页 138，BasSet B1--B2。
    // 输入为非零多项式组（空范围直接返回空）；每轮选最低秩，再筛选约化项。
    // 非零常数可以输入：它会首先被选中并立即结束，返回仅含该常数的列。
    // 输出按秩递增；主变量顺序与比较器优先级相反，供后续回代使用。
    // 不接受含零多项式的范围；选中零时断言失败，不将它作为约化对象。
    template <class Tc,class InputIt>
    std::vector<polynomial_<Tc,lex_<custom_var_order>>> __basset
    (InputIt first,InputIt second)
    {
        if(first==second) return {};
        std::vector<polynomial_<Tc,lex_<custom_var_order>>> F(first,second),B,Temp;
        while(!F.empty())
        {
            //找最小秩的多项式b
            std::size_t min=0;
            for(std::size_t i=1;i<F.size();++i)
                if(rankcompare(F[i],F[min]))    min=i;
            assert(!F[min].empty());
            B.push_back(F[min]);
            // BasSet B2.3：cls 为 0 时结束，不对常数调用 is_reduced。
            if(is_number(F[min])) break;
            //求出新的F
            Temp.clear();
            for(auto i:F)
                if(is_reduced(i,F[min]))  Temp.push_back(i);
            F=std::move(Temp);
            
        }
        return B;
    }

    // 吴特征列算法，王东明《计算机代数》书页 138--139，CharSet C1--C2。
    // 输入为同一比较器下的非零多项式组；反复求基列并加入非零伪余式。
    // 返回升列 C，使输入对 C 的伪余式为零；常数矛盾列表示无公共零点。
    // C 本身不总与输入零集相等，初式消失的分支由后续零点分解处理。
    template <class Tc>
    std::vector<polynomial_<Tc,lex_<custom_var_order>>> charset
    (const std::vector<polynomial_<Tc,lex_<custom_var_order>>>& P)
    {
        if(P.size()==0) return P;
        std::vector<polynomial_<Tc,lex_<custom_var_order>>> R,C;
        std::set<polynomial_<Tc,lex_<custom_var_order>>> F(P.begin(),P.end());
        while(true)
        {
            R.clear();
            C=__basset<Tc>(F.begin(),F.end());
            // std::cout<<"C="<<C<<std::endl;
            // SHOW(C)
            if(is_number(C[0])) return C;
            for(auto f:F)
            {
                // std::cout<<"f1="<<f<<std::endl;
                for(auto c=C.rbegin();c<C.rend();c++)
                {
                    f=prem(f,*c,get_first_var(*c),false);
                }
                // std::cout<<"f2="<<f<<std::endl;
                
                if(!f.empty()) 
                {   
                    R.push_back(f);
                
                }
            }

            // std::cout<<"R="<<R<<std::endl;
            // SHOW(R)
            if (R.empty()) break;
            for (auto &i:R)
                F.insert(std::move(i));
            //std::cout<<"F="<<F<<std::endl;
            
        }
        return C;
    }

    // XY16 Algorithm 2.2（书页 40）：弱相对单纯分解 WRSD。
    // cls 是论文记号，非库函数；非常数 f 的 cls 按 N+1-v_map[主变量] 计算。
    // 输入 T 为零维正则链，满足 cls(T[i])=i+1；f 只含当前链变量。
    // 从 T 的比较器计算 cls；内部递归前缀可以保留原比较器。
    // cls 直接用于 T 的下标，因此 cls(T[i]) 必须从 1 开始连续，不能跳号。
    // 例如比较器 {x1,y,x2,x3} 配主变量 {x3,x2,x1} 会将 x1 算成第 4 层并越界。
    // 返回 (H,G)：分别覆盖 T 上 f 为零和非零的分支，链内方程顺序保持不变。
    // H 使用弱分解的零集契约，不保证 prem(f,H)=0。
    template <class Tc>
    std::pair<
        std::vector<std::vector<polynomial_<Tc,lex_<custom_var_order>>>>,
        std::vector<std::vector<polynomial_<Tc,lex_<custom_var_order>>>>
    > wrsd
    (
        const std::vector<polynomial_<Tc,lex_<custom_var_order>>> &T,
        const polynomial_<Tc,lex_<custom_var_order>> &f
    )
    {
        std::vector<std::vector<polynomial_<Tc,lex_<custom_var_order>>>> H,G;

        // std::cout<<"T="<<T<<std::endl;
        // std::cout<<"f="<<f<<std::endl;

        auto g=f;
        for(auto c=T.rbegin();c<T.rend();c++)
            if(!is_reduced(g,*c))
                g=prem(g,*c,get_first_var(*c));  
        if(f!=g) return wrsd(T,g);

        if(f.empty()) 
        {
            H.push_back(T);
            return std::make_pair(H,G);
        }
        if(is_number(f)) 
        {
            G.push_back(T);
            return std::make_pair(H,G);
        }
        //最后出现变量为x_n(数组下标n-1)
        auto m=T[0].comp().comp.v_map;
        auto n=m.size()+1-m[get_first_var(f)];
        auto x=get_first_var(f);
        //std::cout<<"n="<<n<<std::endl;

        auto res=f;
        for(int i=n-1;i>=0;i--)
        {   
            //std::cout<<"res="<<res<<std::endl;
            res=resultant(res,T[i],get_first_var(T[i]));
        }
        //std::cout<<"res="<<res<<std::endl;
        if(!res.empty())
        {
            G.push_back(T);
            return std::make_pair(H,G);
        }

        //计算正规子结式链
        auto subreschain=subresultant(T[n-1],f,x);
        std::vector<polynomial_<Tc,lex_<custom_var_order>>> regularsubreschain;
        for(auto i=0;i<subreschain.size();i++)
            //由于deg(f)<deg(T[n-1])（因为f关于T约化），所以T[n-1]自动被这个循环加进去了，不用补在最后
            if(degree(subreschain[i],x)==i)
                regularsubreschain.push_back(subreschain[i]);
        //auto v=regularsubreschain.size()-2;

        // std::cout<<"subreschain="<<subreschain<<std::endl;
        // std::cout<<"regularsubreschain="<<regularsubreschain<<std::endl;

        if(n==1)
        {
            auto q=pquo(T[n-1],regularsubreschain[1],x);
            std::vector<polynomial_<Tc,lex_<custom_var_order>>> TT;
            TT.push_back(q);

            //std::cout<<"q="<<q<<std::endl;

            auto W=wrsd(TT,f);
            TT[0]=regularsubreschain[1];
            H.push_back(TT);
            G=W.second;
        }
        else
        {
            std::vector<polynomial_<Tc,lex_<custom_var_order>>> TT(T.begin(),T.begin()+n-1);
            auto W=wrsd(TT,regularsubreschain[0]);
            auto Hi=W.first;
            auto Gi=W.second;
            for(auto t:Gi) 
            {
                t.push_back(T[n-1]);
                G.push_back(t);
            }
            auto i=0;
            while(!Hi.empty())
            {
                i++;
                std::vector<std::vector<polynomial_<Tc,lex_<custom_var_order>>>> Hii,Gii;
                auto R=leadcoeff(regularsubreschain[i]);

                // std::cout<<"S_d="<<regularsubreschain[i]<<std::endl;
                // std::cout<<"R="<<R<<std::endl;

                for(auto h:Hi)
                {
                    W=wrsd(h,R);
                    Hii.insert(Hii.end(),W.first.begin(),W.first.end());
                    Gii.insert(Gii.end(),W.second.begin(),W.second.end());
                }
                for(auto g:Gii)
                {
                    g.push_back(regularsubreschain[i]);
                    H.push_back(g);
                    auto q=pquo(T[n-1],regularsubreschain[i],x);
                    if(degree(q,x)>0)
                    {
                        g.erase(g.end());
                        g.push_back(q);
                        W=wrsd(g,f);
                        G.insert(G.end(),W.second.begin(),W.second.end());
                    }
                }
                Hi=Hii;
                Gi=Gii;
            }
        }

        //需要补上n开始的最后几个多项式
        for(auto &i:H)  i.insert(i.end(),T.begin()+n,T.end());
        for(auto &i:G)  i.insert(i.end(),T.begin()+n,T.end());

        // std::cout<<"H="<<std::endl;
        // for(auto i:H)
        //     std::cout<<i<<std::endl;
        // std::cout<<"G="<<std::endl;
        // for(auto i:G)
        //     std::cout<<i<<std::endl;

        return std::make_pair(H,G);
    }

    // XY16 Algorithm 2.1（书页 34）：相对单纯分解 RSD。
    // 输入顺序和比较器前提与 WRSD 相同：cls(T[i])=i+1。
    // 比较器使 cls 跳号时，不能直接映射到方程下标；否则前缀迭代器可能越界。
    // 返回 (H,G)：H 上 f 的逐次伪余式为零，G 上逐次结式非零；两类覆盖输入零集。
    // 主变量从各方程读取，cls 从比较器计算；不重新排列 T，也不接收根盒。
    template <class Tc>
    std::pair<
        std::vector<std::vector<polynomial_<Tc,lex_<custom_var_order>>>>,
        std::vector<std::vector<polynomial_<Tc,lex_<custom_var_order>>>>
    > rsd
    (
        const std::vector<polynomial_<Tc,lex_<custom_var_order>>> &T,
        const polynomial_<Tc,lex_<custom_var_order>> &f
    )
    {
        // std::cout<<"T="<<T<<std::endl;
        // std::cout<<"g="<<f<<std::endl;
        // getchar();

        auto g=f;
        for(auto c=T.rbegin();c<T.rend();c++)
        {
            g=prem(g,*c,get_first_var(*c));
            if(g.empty()) return {{T},{}};
        }
        g=f;
        for(auto c=T.rbegin();c<T.rend();c++)
        {
            g=resultant(g,*c,get_first_var(*c));
            if(g.empty()) break;
        }
        if(!g.empty()) return {{},{T}};

        //最后出现变量为x_n(数组下标n-1)
        auto m=T[0].comp().comp.v_map;
        auto n=m.size()+1-m[get_first_var(f)];
        if(n!=T.size())
        {
            std::vector<polynomial_<Tc,lex_<custom_var_order>>> temp(T.begin(),T.begin()+n);
            auto W=rsd(temp,f);
            for(auto &i:W.first) i.insert(i.end(),T.begin()+n,T.end());
            for(auto &i:W.second) i.insert(i.end(),T.begin()+n,T.end());
            return W;
        }

        //计算子结式链主系数
        n=T.size();
        auto x=get_first_var(T[n-1]);
        std::vector<polynomial_<Tc,lex_<custom_var_order>>> subreschain;
        if(degree(f,x)>=get_first_deg(T[n-1]))
        {
            subreschain=subresultant(f,T[n-1],x);
        }
        else
        {
            subreschain=subresultant(T[n-1],f,x);
        }
        
        // std::cout<<"subreschain="<<subreschain<<std::endl;

        int i;
        for(i=0;i<subreschain.size();i++)
            if(degree(subreschain[i],x)==i)
            {
                auto R=(i==0)?subreschain[i]:leadcoeff(subreschain[i]);
                for(auto c=T.rbegin()+1;c<T.rend();c++)
                {
                    R=prem(R,*c,get_first_var(*c));
                    if(R.empty()) break;
                }

                if(!R.empty()) break;
            } 

        // std::cout<<"i="<<i<<std::endl;

        auto Rj=(i==0)?subreschain[i]:leadcoeff(subreschain[i]);
        auto R=Rj;
        for(auto c=T.rbegin()+1;c<T.rend();c++)
        {
            //std::cout<<"R="<<R<<std::endl;
            //std::cout<<"*c="<<*c<<std::endl;
            R=resultant(R,*c,get_first_var(*c));
            if(R.empty()) break;
        }
        if(!R.empty())
        {
            // std::cout<<"!=0"<<std::endl;

            auto ft1=subreschain[i];
            for(auto c=T.rbegin()+1;c<T.rend();c++)
                ft1=prem(ft1,*c,get_first_var(*c));

            auto ft2=pquo(T[n-1],ft1,x);
            // std::cout<<"ft1="<<ft1<<std::endl;
            // std::cout<<"ft2="<<ft2<<std::endl;
            auto T1=T,T2=T;
            T1[n-1]=ft1;
            T2[n-1]=ft2;
            auto W1=rsd(T1,f),W2=rsd(T2,f);
            W1.first.insert(W1.first.end(),W2.first.begin(),W2.first.end());
            W1.second.insert(W1.second.end(),W2.second.begin(),W2.second.end());
            return W1;
        }
        else
        {
            // std::cout<<"=0"<<std::endl;
            auto temp=T;
            temp.pop_back();
            auto L=rsd(temp,Rj);
            std::pair<
            std::vector<std::vector<polynomial_<Tc,lex_<custom_var_order>>>>,
            std::vector<std::vector<polynomial_<Tc,lex_<custom_var_order>>>>
            > W1;
            for(auto i:L.first)
            {
                i.push_back(T[n-1]);
                auto W2=rsd(i,f);
                W1.first.insert(W1.first.end(),W2.first.begin(),W2.first.end());
                W1.second.insert(W1.second.end(),W2.second.begin(),W2.second.end());
            }
            for(auto i:L.second)
            {
                i.push_back(T[n-1]);
                auto W2=rsd(i,f);
                W1.first.insert(W1.first.end(),W2.first.begin(),W2.first.end());
                W1.second.insert(W1.second.end(),W2.second.begin(),W2.second.end());
            }
            return W1;
        }
    }

    // 在已合法排列的三角列上，用初式的逐次结式检查正则性。
    // 调用者先保证三角结构；此函数不检查平方自由性或枚举实根。
    template <class Tc>
    bool is_regular
    (const std::vector<polynomial_<Tc,lex_<custom_var_order>>> &T)
    {
        //判断三角列是否为正则三角列
        for(int n=1;n<T.size();n++)
        {
            auto res=leadcoeff(T[n]);
            for(int i=n-1;i>=0;i--)
            {
                res=resultant(res,T[i],get_first_var(T[i]));
                if(res.empty()) return false;
            }
        }
        return true;
    }

    template <class Tc>
    std::vector<std::vector<polynomial_<Tc,lex_<custom_var_order>>>> ZDtoRC
    (const std::vector<polynomial_<Tc,lex_<custom_var_order>>> &T)
    {
        std::vector<std::vector<polynomial_<Tc,lex_<custom_var_order>>>> G;
        //找出最大的正则子链对应的k
        int k=1;
        for(;k<T.size();k++)
        {
            auto res=leadcoeff(T[k]);
            for(int i=k-1;i>=0;i--)
            {
                res=resultant(res,T[i],get_first_var(T[i]));
                if(res.empty()) break;
            }
            if(res.empty()) break;
        }
        //std::cout<<"k="<<k<<std::endl;
        if(k==T.size())
        {
            G.push_back(T);
            return G;
        } 

        std::vector<polynomial_<Tc,lex_<custom_var_order>>> Tk(T.begin(),T.begin()+k);

        auto W=wrsd(Tk,leadcoeff(T[k]));

        if(W.second.empty()) return G;
        for(auto R:W.second)
        {
            //化简多项式,可能可以放到wrsd里面
            R=sqrfree(R);
            //std::cout<<"R="<<R<<std::endl;

            R.insert(R.end(),T.begin()+k,T.end());
            auto Z=ZDtoRC(R);
            G.insert(G.end(),Z.begin(),Z.end());
        }
        return G;
    }

    template <class Tc>
    std::vector<std::vector<polynomial_<Tc,lex_<custom_var_order>>>> Wucharset
    (const std::vector<polynomial_<Tc,lex_<custom_var_order>>> &P)
    {
        std::vector<std::vector<polynomial_<Tc,lex_<custom_var_order>>>> T;
        auto C=charset(P);

        for(auto &i:C)
        {
            //化简多项式,可能可以放到charset里
            i=sqrfree(i);
            //std::cout<<"C="<<i<<std::endl;
        }
        
        if(is_number(C[0])) return T;

        T.push_back(C);
        for(auto f:C)
        {
            if(is_number(leadcoeff(f))) continue;

            auto P_temp=P;
            P_temp.push_back(leadcoeff(f));
            auto T_temp=Wucharset(P_temp);
            T.insert(T.end(),T_temp.begin(),T_temp.end());
        }
        return T;
    }

    template <class Tc>
    std::vector<std::vector<polynomial_<Tc,lex_<custom_var_order>>>> GRDforZD
    (const std::vector<polynomial_<Tc,lex_<custom_var_order>>> &P)
    {
        std::vector<std::vector<polynomial_<Tc,lex_<custom_var_order>>>> T;

        auto C=Wucharset(P);

        //std::cout<<"charset"<<std::endl;

        for(auto i:C)
        {
            auto W=ZDtoRC(i);

            T.insert(T.end(),W.begin(),W.end());
        }
        return T;
    }

    // XY16 Algorithm 3.1（书页 62）：将零维正则链分解为平方自由正则链。
    // 输入按 cls 递增排列；每层主变量由 get_first_var 取得，比较器由输入携带。
    // 返回各链的零集之并等于原零集，链内仍按原主变量顺序排列；空链返回 {T}。
    // 检查包含第一条方程；依赖 WRSD 和子结式处理重根，不在此选择某个实根。
    template <class Tc>
    std::vector<std::vector<polynomial_<Tc,lex_<custom_var_order>>>> RCtoSqrfree
    (const std::vector<polynomial_<Tc,lex_<custom_var_order>>> &T)
    {
        std::vector<std::vector<polynomial_<Tc,lex_<custom_var_order>>>> TT;
        //找出最大的不符合非平方的k
        // XY16 Algorithm 3.1, lines 1--3: the first polynomial also
        // participates; only k < 0 means every level is squarefree.
        int k=static_cast<int>(T.size())-1;
        for(;k>=0;k--)
        {
            auto res=discriminant(T[k],get_first_var(T[k]));
            for(int i=k-1;i>=0;i--)
            {
                res=resultant(res,T[i],get_first_var(T[i]));
                if(res.empty()) break;
            }
            if(res.empty()) break;
        }

        if(k<0)
        {
            TT.push_back(T);
            return TT;
        }

        std::vector<polynomial_<Tc,lex_<custom_var_order>>> Tk(T.begin(),T.begin()+k);
        auto W=wrsd(Tk,discriminant(T[k],get_first_var(T[k])));
        for(auto G:W.second)
        {
            G.insert(G.end(),T.begin()+k,T.end());
            auto temp=RCtoSqrfree(G);
            TT.insert(TT.end(),temp.begin(),temp.end());
        }

        //line 9:计算子结式链主系数
        auto x=get_first_var(T[k]);
        auto dtk=derivative(T[k],x);
        std::vector<polynomial_<Tc,lex_<custom_var_order>>> subreschain;
        if(degree(dtk,x)>get_first_deg(T[k]))
        {
            subreschain=subresultant(dtk,T[k],x);
        }
        else
        {
            subreschain=subresultant(T[k],dtk,x);
        }
        
        // std::cout<<"subreschain="<<subreschain<<std::endl;

        int i;
        for(i=0;i<subreschain.size();i++)
            if(degree(subreschain[i],x)==i)
            {
                auto R=(i==0)?subreschain[i]:leadcoeff(subreschain[i]);
                for(auto c=Tk.rbegin();c<Tk.rend();c++)
                {
                    R=prem(R,*c,get_first_var(*c));
                    if(R.empty()) break;
                }

                if(!R.empty()) break;
            }
        
        auto Rj=(i==0)?subreschain[i]:leadcoeff(subreschain[i]);
        auto R=Rj;
        for(auto c=Tk.rbegin();c<Tk.rend();c++)
        {
            //std::cout<<"R="<<R<<std::endl;
            //std::cout<<"*c="<<*c<<std::endl;
            R=resultant(R,*c,get_first_var(*c));
            if(R.empty()) break;
        }
        if(!R.empty())
        {
            auto ft1=subreschain[i];
            for(auto c=Tk.rbegin();c<Tk.rend();c++)
                ft1=prem(ft1,*c,get_first_var(*c));

            auto ft2=pquo(T[k],ft1,x);
            Tk.push_back(ft2);
            Tk.insert(Tk.end(),T.begin()+k+1,T.end());
            auto temp=RCtoSqrfree(Tk);
            TT.insert(TT.end(),temp.begin(),temp.end());
        }
        else
        {
            auto W=wrsd(Tk,Rj);
            for(auto G:W.first)
            {
                G.insert(G.end(),T.begin()+k,T.end());
                auto temp=RCtoSqrfree(G);
                TT.insert(TT.end(),temp.begin(),temp.end());
            }
            if(i!=0)
            {
                for(auto G:W.second)
                {
                    G.insert(G.end(),T.begin()+k,T.end());
                    auto temp=RCtoSqrfree(G);
                    TT.insert(TT.end(),temp.begin(),temp.end());
                }
            }
        }
        return TT;
    }

    template <class Tc>
    std::vector<std::vector<std::vector<polynomial_<Tc,lex_<custom_var_order>>>>> GRDforZDSAS
    (const std::vector<std::vector<polynomial_<Tc,lex_<custom_var_order>>>> &S)
    {
        std::vector<std::vector<std::vector<polynomial_<Tc,lex_<custom_var_order>>>>> TT,U,V;
        auto P=S[0];
        auto O=GRDforZD(P);
        std::vector<std::vector<polynomial_<Tc,lex_<custom_var_order>>>> OO;
        
        auto G1=S[1],G2=S[2],H=S[3];

        // std::cout<<"P="<<S[0]<<std::endl;
        // for(auto i:O)
        //     std::cout<<"O="<<i<<std::endl;

        for(auto h:H)
        {
            for(auto T:O)
            {
                // std::cout<<"T="<<T<<std::endl;
                // std::cout<<"h="<<h<<std::endl;
                
                auto BPh=h;
                for(auto c=T.rbegin();c<T.rend();c++)
                {
                    BPh=resultant(BPh,*c,get_first_var(*c));
                    if(BPh.empty()) break;
                }

                if(BPh.empty())
                {
                    //std::cout<<"BPh=0"<<std::endl;
                    auto W=wrsd(T,h);
                    
                    // for(auto i:W.first)
                    //     std::cout<<"w1="<<i<<std::endl;
                    // for(auto i:W.second)
                    //     std::cout<<"w2="<<i<<std::endl;

                    //getchar();

                    OO.insert(OO.end(),W.second.begin(),W.second.end());
                }
                else 
                {
                    // std::cout<<"BPh!=0"<<std::endl;
                    OO.push_back(T);
                }
            }
            O=OO;
            OO.clear();
        }

        // for(auto i:O)
        //     std::cout<<"Th="<<i<<std::endl;

        for(auto g:G2)
        {
            for(auto T:O)
            {
                auto BPg=g;
                for(auto c=T.rbegin();c<T.rend();c++)
                {
                    BPg=resultant(BPg,*c,get_first_var(*c));
                    if(BPg.empty()) break;
                }

                if(BPg.empty())
                {
                    auto W=wrsd(T,g);
                    OO.insert(OO.end(),W.second.begin(),W.second.end());
                }
                else OO.push_back(T);
            }
            O=OO;
            OO.clear();
        }

        for(auto T:O)
            U.push_back({T,{},G2,{}});
        for(auto g:G1)
        {
            for(auto u:U)
            {
                auto T=u[0];
                auto BPg=g;
                for(auto c=T.rbegin();c<T.rend();c++)
                {
                    BPg=resultant(BPg,*c,get_first_var(*c));
                    if(BPg.empty()) break;
                }

                if(BPg.empty())
                {
                    auto W=wrsd(T,g);
                    for(auto c:W.first)
                        V.push_back({c,{},u[2],{}});

                    u[2].push_back(g);
                    for(auto c:W.second)
                        V.push_back({c,{},u[2],{}});
                }
                else 
                {
                    u[2].push_back(g);
                    V.push_back(u);
                }
            }
            U=V;
            V.clear();
        }
        for(auto u:U)
        {
            auto temp=RCtoSqrfree(u[0]);
            for(auto t:temp)
                TT.push_back({t,{},u[2],{}});
        }
        return TT;
    }

}
#endif
