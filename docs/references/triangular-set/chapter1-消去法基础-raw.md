                                                  Chapter 1

> 历史阅读辅助：保留原整理日期和观点，尚未逐条校正，不作为当前接口、算法选择或正确性结论。OCR、译述和页码以原文为准；当前复核见 [CAD 文档入口](../../cad/README.md)。

                      Basics of Elimination Method



            Pseudo-division and resultant are two basic tools used by many elimination
            methods and are also frequently used in many algorithms in this book. So,
            we begin with an introduction to some related concepts and results.
               If not speciﬁed in this chapter, R is a domain and univariate polynomials
            are in R[x]. The degree of f ∈ R[x] is denoted by deg(f, x) or deg(f ).


            1.1      Pseudo-division

            If K is a ﬁeld, the Euclidean division in the Euclidean domain K[x] is well-
            known. For two polynomials f and g = 0 in K[x], there exist q, r ∈ K[x]
            such that f = qg + r and deg(r, x) < deg(g, x). The polynomials q and r
            are called respectively the quotient and remainder of f divided by g and
            denoted by quo(f, g) and rem(f, g), respectively.
                If R is a domain, the concept of division in R[x] is generalized to the
            so-called pseudo-division, for the element in R is not invertible in general.
                Suppose

                                            
                                            m                       
                                                                    l
                                      f=          ai xi and g =           bi xi
                                            i=0                     i=0

            are polynomials in R[x] with m ≥ l. Construct a matrix as follows.
                                        ⎛                               ⎞
                                            · · · b1 b0
                                            bl
                                      ⎜      bl · · · b1 b0             ⎟
                                      ⎜                                 ⎟
                                      ⎜           ..        ..          ⎟
                                   M =⎜              .          .       ⎟,          (1.1)
                                      ⎜                                 ⎟
                                      ⎝                bl · · · b1 b0 ⎠
                                        am am−1 · · · · · · · · · a1 a0

May 16, 2016 15:58   ws-book9x6      Automated Inequality Proving and Discovering     mybook   page 2




            2                     Automated Inequality Proving and Discovering


            where all the other entries are zero except those of the coeﬃcients of f and
            g. The ith column of M can be viewed as indexed by xm−i+1 . That is to
            say,
                                      ⎛ m ⎞ ⎛ m−l ⎞
                                          x             x    g
                                      ⎜ xm−1 ⎟ ⎜ xm−l−1 g ⎟
                                      ⎜        ⎟ ⎜              ⎟
                                      ⎜        ⎟ ⎜              ⎟
                                  M · ⎜ ... ⎟ = ⎜         ..
                                                           .    ⎟.
                                      ⎜        ⎟ ⎜              ⎟
                                      ⎝ x ⎠ ⎝             g     ⎠
                                                 1                   f
            If R is a ﬁeld or, at least, bl is invertible, perform Gaussian elimination on
            M to get the following matrix
                                     ⎛                               ⎞
                                        bl · · · b1 b0
                                     ⎜ bl · · · b1 b0                ⎟
                                     ⎜                               ⎟
                                     ⎜           ..       ..         ⎟
                                     ⎜              .        .       ⎟.               (1.2)
                                     ⎜                               ⎟
                                     ⎝                bl · · · b1 b0 ⎠
                                           0 · · · · · · 0 rl−1 · · · r0

            Then, the last row of the matrix is the remainder, i.e. r = l−1          i
                                                                             i=0 ri x is the
            remainder of f divided by g, denoted as r = rem(f, g).
                If bl is not invertible, we apply fraction-free Gaussian elimination on M
            as follows: First, multiply the last row by bl and minus the product of the
            ﬁrst row by am . Suppose the ith step (1 ≤ i < m − l + 1) is completed and
            the ith entry of the last row is ci , multiply the last row by bl and minus
            the product of the ith row by ci . After m − l + 1 such transformations, M
            is transformed into a matrix of the form (1.2).

            Deﬁnition 1.1. Let notations be as above. If M is transformed into a
            matrix of the form (1.2) by the above procedure via fraction-free Gaussian
                                                l−1
            elimination, we then deﬁne r = i=0 ri xi as the pseudo-remainder of f
            pseudo-divided by g and denote it by prem(f, g, x) or prem(f, g).
                Obviously, there exists a polynomial q ∈ R[x] such that
                                               bm−l+1
                                                l     f = qg + r.                     (1.3)
            We deﬁne q as the pseudo-quotient of f pseudo-divided by g and denote
            it by pquo(f, g, x) or pquo(f, g). The formula (1.3) is called the pseudo-
            remainder formula.

               If ci = 0 for some i in the above procedure, bl is a common divisor of
            the two sides of the pseudo-remainder formula. Obviously, if we modify the

May 16, 2016 15:58   ws-book9x6    Automated Inequality Proving and Discovering        mybook   page 3




                                       Basics of Elimination Method                         3


            above procedure and do not multiply the last row by bl when ci = 0, the
            exponent of bl in formula (1.3) can be decreased.

            Example 1.1. Suppose
                                  f = 2x3 − x2 + 1, g = 3x2 + x − 1.
            We compute the (pseudo-)remainder of f (pseudo-)divided by g. Construct
            the matrix
                                       ⎛                ⎞
                                         3 1 −1 0
                                  M = ⎝ 0 3 1 −1 ⎠ .
                                         2 −1 0 1
            Apply Gaussian elimination and fraction-free Gaussian elimination on M
            to transform it respectively into
                           ⎛                  ⎞           ⎛             ⎞
                              3 1 −1 0                      3 1 −1 0
                     M1 = ⎝ 0 3 1 −1 ⎠ and M2 = ⎝ 0 3 1 −1 ⎠ .
                              0 0 11/9 4/9                  0 0 11 4
            Therefore
                                            11    4
                          rem(f, g, x) =       x + , prem(f, g, x) = 11x + 4.
                                             9    9
               Pseudo-remainders can also be expressed explicitly. Suppose M =
            (mij )t×s is a matrix over R where t ≤ s. Deﬁne the determinant poly-
            nomial of M to be
                detpol(M ) = det(M (t) )xs−t + det(M (t+1) )xs−t−1 + · · · + det(M (s) ),
            where M (j) is a t×t submatrix formed by the ﬁrst t−1 and the jth columns
            of M and det(M (j) ) is the determinant of M (j) .
               Suppose
                                            
                                            ni
                                    Ai =          aij xni −j   (1 ≤ i ≤ k)
                                            j=0

            is a ﬁnite list of polynomials and t = 1 + max(n1 , . . . , nk ). We denote by
            mat(A1 , . . . , Ak ) the matrix (mij )k×t , where mij is the coeﬃcient of Ai in
            xt−j .

            Deﬁnition 1.2. The determinant polynomial of a list of polynomials
            A1 , . . . , Ak is deﬁned to be
                           detpol(A1 , . . . , Ak ) = detpol(mat(A1 , . . . , Ak )).

May 16, 2016 15:58   ws-book9x6      Automated Inequality Proving and Discovering             mybook    page 4




            4                     Automated Inequality Proving and Discovering


            Example 1.2. Suppose

                        A1 = x3 + 2x + 5, A2 = 3x2 − x − 6, A3 = −x4 + x3 .

            Then t = 5, k = 3 and
                                                          ⎛        ⎞
                                                       0 1 0 2 5
                                mat(A1 , A2 , A3 ) = ⎝ 0 0 3 −1 −6 ⎠ .
                                                      −1 1 0 0 0

            Thus

                                detpol(A1 , A2 , A3 )
                                0 1 0       0 1 2     0 1 5
                            =   0 0 3 x2 + 0 0 −1 x + 0 0 −6
                               −1 1 0       −1 1 0    −1 1 0
                            = −3x2 + x + 6.

                The following proposition is important, which gives an explicit expres-
            sion of pseudo-remainder.

            Proposition 1.1. Suppose f, g ∈ R[x] and deg(f ) = m ≥ l = deg(g) > 0,
            then

                         detpol(xm−l g, xm−l−1 g, . . . , xg, g, f ) = prem(f, g, x).
                                             m         i                 l       i
            Proof. Assume f =                i=0 ai x and g =            i=0 bi x . It is easy to see
                        m−l         m−l−1
            that mat(x        g, x        g, . . . , xg, g, f ) is just the matrix M in Eq. (1.1).
            Suppose prem(f, g, x) = l−1                 i
                                              i=0 ri x , we need to prove that rj = M
                                                                                             (m+1−j)

            for j = 0, 1, . . . , l − 1. By the procedure of pseudo-division, M            (m+1−j)
                                                                                                   is
            transformed into
                                                      ⎛                 ⎞
                                                 bl · · · b1 b0
                                               ⎜ bl · · · b1            ⎟
                                               ⎜                        ⎟
                                               ⎜                    .. ⎟
                                      (m+1−j)             .       .
                                    M         =⎜            ..          ⎟.
                                               ⎜                        ⎟
                                               ⎝               bl · · · ⎠
                                                          0 · · · · · · 0 rj

                                                                     (m+1−j)
            Obviously, bm−l+1
                        l     det(M (m+1−j) ) = det(M                          ) = bm−l+1
                                                                                    l     rj . That
            completes the proof.                                                                 

May 16, 2016 15:58    ws-book9x6   Automated Inequality Proving and Discovering             mybook    page 5




                                       Basics of Elimination Method                              5


            1.2      Resultants

            Generally speaking, a resultant of two univariate polynomials f, g ∈ R[x] is
            a polynomial in the coeﬃcients of f and g such that the polynomial being
            zero is a necessary (and suﬃcient) condition for the two polynomials to
            have common zeros. Herein, a common zero x̄ of f and g is an element of a
            certain extension ﬁeld of the quotient ﬁeld of R such that f (x̄) = g(x̄) = 0.
                The theory of resultant is a classical theory of elimination methods.
            There exist many well-known kinds of resultants, such as the Sylvester
            resultant, the Bézout resultant, the Dixon resultant and the Macaulay re-
            sultant. For our purpose in this book, we only introduce the Sylvester
            resultant, which not only has elegant properties but also fully demonstrate
            the idea and techniques of classical elimination methods. Moreover, the
            theory of subresultants introduced in the next subsection is based on the
            Sylvester matrix. So, in this book, if not speciﬁed, resultant always means
            the Sylvester resultant.
                Suppose
                             f = a0 xm + a1 xm−1 + · · · + am−1 x + am ,
                                                                                     (1.4)
                             g = b0 xl + b1 xl−1 + · · · + bl−1 x + bl ,
            are two polynomials in R[x] with positive degrees. Construct an (m + l) ×
            (m + l) square matrix
                                  ⎛                                 ⎞⎫
                                     a0 a1 · · · am                      ⎪
                                                                         ⎪
                                  ⎜                                 ⎟    ⎪
                                                                         ⎬
                                  ⎜     a0 a1 · · · am              ⎟ l
                                  ⎜          .. ..          ..      ⎟⎪
                                  ⎜                                 ⎟⎪
                                  ⎜            .   .           .    ⎟⎪
                                  ⎜                                 ⎟⎭
                                  ⎜              a0 a1 · · · am ⎟
                             S=⎜                                    ⎟⎫ ,             (1.5)
                                  ⎜ b0 b1 · · · bl                  ⎟⎪
                                  ⎜                                 ⎟⎪
                                  ⎜     b0 b1 · · · bl              ⎟⎪   ⎬
                                  ⎜                                 ⎟ m
                                  ⎜          .. ..          ..      ⎟⎪
                                  ⎝            .   .           .    ⎠⎪   ⎪
                                                                         ⎭
                                                 b0 b1 · · · bl
            where all the other entries except those of the coeﬃcients of f and g are
            zero. We call this matrix the Sylvester matrix of f and g with respect to
            x. It is clear that
                   ⎛ m+l−1 ⎞
                      x
                   ⎜ xm+l−2 ⎟
                   ⎜          ⎟
                   ⎜     ..   ⎟
               S·⎜        .   ⎟ = (xl−1 f, xl−2 f, . . . , xf, f, xm−1 g, xm−2 g, . . . , xg, g)T .
                   ⎜          ⎟
                   ⎝ x ⎠
                         1

May 18, 2016 10:21   ws-book9x6      Automated Inequality Proving and Discovering      mybook   page 6




            6                     Automated Inequality Proving and Discovering


            Deﬁnition 1.3. The determinant of Sylvester matrix S is called the
            Sylvester resulatnt of f and g with respect to x and denoted by res(f, g, x).

                The resultant res(f, g, x) = det(S) is homogeneous, which is of degree l
            in ai and of degree m in bi .
                We extend the deﬁnition to cover some special cases. Deﬁne
                                 ⎧ l
                                 ⎪
                                 ⎪ a0 , if f ∈ R, g ∈ R,
                                 ⎨ m
                                   b0 , if g ∈ R, f ∈ R,
                  res(f, g, x) =                                                   (1.6)
                                 ⎪
                                 ⎪ 0,   if f = g = 0,
                                 ⎩
                                   1, if f, g ∈ R, f and g are not both zero.

            Lemma 1.1. Suppose f and g are polynomials of positive degrees in the
            form of (1.4), there exist nonzero polynomials p, q ∈ R[x] such that

                                            pf + qg = res(f, g, x),                    (1.7)

            and deg(p, x) < deg(g, x), deg(q, x) < deg(f, x).

            Proof. Denote the Sylvester matrix of f and g by S. Multiply the ith
            column by xm+l−i and add to the last column of S for all i(1 ≤ i ≤ m+l−1).
            That gives
                              ⎛                                        ⎞
                                a0 a1 · · · · · · am           xl−1 f
                              ⎜    a 0 a 1 · · · · · · am      xl−2 f ⎟
                              ⎜                                        ⎟
                              ⎜        .. ..              ..       .. ⎟
                              ⎜           .     .            .      . ⎟
                              ⎜                                        ⎟
                              ⎜                                        ⎟
                             ⎜              a0 a1 · · · am−1 f ⎟
                        S =⎜                                           ⎟.
                              ⎜ b0 b1 · · · · · · bl           xm−1 g ⎟
                              ⎜                                        ⎟
                              ⎜    b0 b1 · · · · · · bl        xm−2 g ⎟
                              ⎜                                        ⎟
                              ⎜        .. ..              ..      .. ⎟
                              ⎝           .     .            .     . ⎠
                                             b0 b1 · · · bl−1     g

            Then

                                      res(f, g, x) = det(S) = det(S  ).

            Expanding det(S  ) by minors along the last column and collecting the terms
            containing f and g, respectively, will give the formula (1.7).
                We prove that p and q are nonzero. If res(f, g, x) = 0, the conclusion is
            clear. If res(f, g, x) = 0, assume

                     p(x) = ul−1 xl−1 + · · · + u0 , q(x) = vm−1 xm−1 + · · · + v0 .

May 16, 2016 15:58   ws-book9x6   Automated Inequality Proving and Discovering        mybook   page 7




                                      Basics of Elimination Method                        7


            Then                              ⎛  ⎞ ⎛ ⎞
                                            ul−1      0
                                          ⎜ . ⎟ ⎜.⎟
                                          ⎜ .. ⎟ ⎜ .. ⎟
                                          ⎜      ⎟ ⎜ ⎟
                                          ⎜      ⎟ ⎜ ⎟
                                        T ⎜ u0 ⎟     ⎜0⎟
                                       S ·⎜      ⎟ = ⎜ ⎟.
                                          ⎜ vm−1 ⎟ ⎜ 0 ⎟
                                          ⎜      ⎟ ⎜ ⎟
                                          ⎜ .. ⎟ ⎜ .. ⎟
                                          ⎝ . ⎠ ⎝.⎠
                                             v0       0
            As det(S T ) = res(f, g, x) = 0, the equations have nonzero solutions. That
            completes the proof.                                                     
            Theorem 1.1. Suppose f and g are polynomials of the form (1.4) with
            positive degrees. Assume a0 and b0 are not both zero, then f and g have
            non-trivial common divisors if and only if res(f, g, x) = 0.
            Proof. By Lemma 1.1, a common divisor of f and g must be a divisor
            of res(f, g, x). If res(f, g, x) = 0, res(f, g, x) must be a nonzero element of
            R. So, f and g do not have non-trivial common divisors.
                On the contrary, assume res(f, g, x) = 0. By Lemma 1.1, there exist p
            and q such that pf = −qg. Without loss of generality (w.l.o.g.), assume
            a0 = 0. If f and g do not have non-trivial common divisors, then f divides
            q. This is a contradiction since deg(q, x) < deg(f, x).                       
                It is not diﬃcult to verify the following proposition.
            Proposition 1.2. Let f and g be as in (1.4). The resultant of f and g
            satisﬁes the following three properties.
            (1) If f ∈ R or/and g ∈ R, res(f, g, x) satisﬁes Eq. (1.6);
            (2) res(f, g, x) = (−1)ml res(g, f, x);
            (3) Assume l ≥ m > 0 and r = rem(g, f ) is the remainder of g divided by
                f in the quotient ﬁeld of R. Let t = deg(r, x), then
                                        res(f, g, x) = al−t
                                                        0 res(f, r, x).

                The three items of Proposition 1.2 give naturally an algorithm for com-
            puting resultants. In fact, the algebraic form satisfying the three properties
            is unique.
            Lemma 1.2. Assume f, g are polynomials in R[x]. If R(f, g, x) is a poly-
            nomial in the coeﬃcients of f and g over a certain extension ﬁeld of the
            quotient ﬁeld of R and R(f, g, x) satisﬁes the three properties of Proposition
            1.2, then R(f, g, x) = res(f, g, x).

May 16, 2016 15:58    ws-book9x6      Automated Inequality Proving and Discovering                  mybook   page 8




            8                      Automated Inequality Proving and Discovering


            Proof. By induction on min(deg(f ), deg(g)), the conclusion is almost
            obvious.                                                           
            Theorem 1.2. Suppose f and g are as in (1.4) and αi (i = 1, . . . , m) and
            βj (j = 1, . . . , l) are respectively the zeros of f and g in a certain extension
            ﬁeld of the quotient ﬁeld of R. Then
                                                 
                                                 m                     l
                              res(f, g, x) = al0    g(αi ) = (−1)ml bm
                                                                     0    f (βj )
                                                  i=1                          j=1

                                                      
                                                      m 
                                                        l
                                           = al0 bm
                                                  0             (αi − βj ).
                                                      i=1 j=1
            Herein, the products are deﬁned to be 0 if f = g = 0 and to be 1 if f ∈ R
            and g ∈ R are not both zero.
            Proof. We only need to prove the ﬁrst equality. It is easy to verify that
            the ﬁrst product satisﬁes the three properties in Proposition 1.2. Then the
            conclusion follows by Lemma 1.2.                                         
            Corollary 1.1. Let notations be as in Theorem 1.2. The discriminant of
            f is deﬁned to be
                                                  
                              dis(f, x) = a20 m−2     (αi − αj )2 .
                                                           1≤j<i≤m
                                            m (m−1)
                            
            Then res(f, f , x) = (−1)          2      a0 dis(f, x).


            1.3      Subresultants

            The theory of subresultants is one of the classical constructible theory,
            which is the basis of many well-known algorithms in algorithmic algebra
            and computational algebraic geometry. We introduce some basic concepts
            and results of subresultants in this section.
               Suppose f and g are polynomials in (1.4). Without loss of generality,
            we assume that m ≥ l > 0 in this section.
            Deﬁnition 1.4. For each j(0 ≤ j < l), set
                           S j = mat(xl−j−1 f, . . . , xf, f, xm−j−1 g, . . . , xg, g),
            which is an (m + l − 2j) × (m + l − j) matrix. We call
                                                                   
                                                                   j
                                                                               (m+l−2j+i)
                      Sj = subresj (f, g) = detpol(S j ) =               det(S j            )xj−i
                                                                   i=0

May 16, 2016 15:58   ws-book9x6   Automated Inequality Proving and Discovering     mybook   page 9




                                      Basics of Elimination Method                     9


            the jth subresultant of f and g with respect to x and Rj = det(S m+l−2j
                                                                               j    )
            the jth principal subresultant coeﬃcient (PSC) of f and g with respect to
            x.
                If m > l + 1, the deﬁnition of the jth subresultant Sj and the PSC Rj
            of f and g with respect to x is extended as follows:
                    Sl = bm−l−1
                           0     g, Rl = bm−l
                                            0   ; Sj = Rj = 0, l < j < m − 1.
                Obviously, deg(Sj , x) ≤ j. Sj is said to be defective of degree t if
            deg(Sj , x) = t < j, and regular otherwise.
               It is easy to see that S0 = R0 is the resultant of f and g with respect
            to x.
            Theorem 1.3. Let f, g ∈ R[x] be as in (1.4) and m = deg(f, x) ≥
            deg(g, x) = l > 0. For the jth subresultant Sj (0 ≤ j < m − 1) of f
            and g with respect to x, there exist polynomials pj , qj ∈ R[x] such that
                                          pj f + qj g = Sj ,
            where deg(pj , x) < l − j and deg(qj , x) < m − j.
            Proof. If l ≤ j < m − 1, the conclusion is clear. We consider the case
            that j < l. It is easy to verify that the determinant of the following matrix
                    ⎛                                            ⎞ ⎫
                      a0 a1 · · · · · · am              xl−j−1 f     ⎪
                                                                     ⎪
                    ⎜
                    ⎜     a0 a1 · · · · · · am          xl−j−2 f ⎟
                                                                 ⎟
                                                                     ⎬
                    ⎜          .    .             .          .   ⎟     l − j rows
                    ⎜            .. ..              ..       ..  ⎟ ⎪
                    ⎜                                            ⎟ ⎪ ⎭
                    ⎜                                            ⎟
                    ⎜               a0 a1 · · · am−j−1     f     ⎟
                    ⎜                                            ⎟
                    ⎜ b0 b1 · · · · · · bl              xm−j−1 g ⎟ ⎫
                    ⎜                                            ⎟ ⎪ ⎪
                    ⎜     b0 b1 · · · · · · bl          xm−j−2 g ⎟   ⎬
                    ⎜                                            ⎟
                    ⎜          .. ..              ..        ..   ⎟ ⎪ m − j rows
                    ⎝             .   .              .       .   ⎠ ⎪ ⎭
                                    b0 b1 · · · bl−j−1      g
            is Sj . Expanding the matrix by minors along the last column gives the
            result.                                                                    
            Proposition 1.3. Let f, g ∈ R[x] be as in (1.4) and m = deg(f, x) ≥
            deg(g, x) = l > 0. Then
                                 Sl−1 = (−1)m−l+1 prem(f, g, x).
                In the following subsections, we introduce some main results on subre-
            sultants. The structure of the subsections and the idea of proofs are mainly
            taken from [Mishra (1993)]. However, the main content is from [Xia (2003)]
            where the subresultant chain theorem is corrected and thus some lemmas
            are modiﬁed. See also [Wang and Xia (2004)].

May 16, 2016 15:58   ws-book9x6     Automated Inequality Proving and Discovering                    mybook   page 10




            10                    Automated Inequality Proving and Discovering


            1.3.1    The Habicht Theorem
            Deﬁnition 1.5. Let f, g ∈ R[x] be as in (1.4) and m = deg(f, x) ≥
            deg(g, x) = l > 0. Set
                                            
                                              m − 1, if m > l,
                                      μ=
                                              l,       otherwise.
            Let Sμ+1 = f , Sμ = g, and Sj (0 ≤ j < μ) be the jth subresultant of f and
            g with respect to x. The sequence of polynomials
                                        Sμ+1 , Sμ , Sμ−1 , . . . , S0
            in R[x] is called the subresultant chain of f and g with respect to x. The
            chain is said to be regular if every Sj is regular, and defective otherwise.
                Let                         
                                              lc(Sj , x), if Sj is regular,
                    Rμ+1 = 1, and Rj =                                      0 ≤ j ≤ μ.
                                              0,           otherwise,
            The sequence
                                           Rμ+1 , Rμ , . . . , R0
            is called the PSC chain of f and g with respect to x.
               We ﬁrst study the subresultant chain of two polynomials in symbolic
            coeﬃcients of degrees n + 1 and n > 0, respectively. Let
                                A = a0 xn+1 + a1 xn + · · · + an+1 ,
                                                                                           (1.8)
                                B = b0 xn + b1 xn−1 + · · · + bn ,
            which are viewed as polynomials in Z[a0 , . . . , an+1 , b0 , . . . , bn ][x].
            Lemma 1.3. Let A and B be as above. Then
            (a) Sn−1 (A, B) = prem(A, B, x);
                                2(n−i−1)
            (b) For i < n − 1, b0        subresi (A, B) = subresi (B, prem(A, B, x)).
            Proof.     Firstly,
                      Sn−1 (A, B) = detpol(A, xB, B) = (−1)2 detpol(xB, B, A)
                                 = prem(A, B, x).
            Secondly, let R = prem(A, B, x), then
               subresi (A, B) = detpol(xn−i−1 A, . . . , A, xn−i B, . . . , B)
                                     −2 (n−i)
                                  = b0          detpol(xn−i−1 b20 A, . . . , b20 A, xn−i B, . . . , B)
                                     −2 (n−i)
                                  = b0          detpol(xn−i−1 R, . . . , R, xn−i B, . . . , B)
                                     −2 (n−i)
                                  = b0          detpol(xn−i B, . . . , B, xn−i−1 R, . . . , R)
                                     −2 (n−i)+2
                                  = b0            detpol(xn−i−2 B, . . . , B, xn−i−1 R, . . . , R).

May 16, 2016 15:58     ws-book9x6        Automated Inequality Proving and Discovering                 mybook     page 11




                                             Basics of Elimination Method                                   11


            Therefore
                 2 (n−i−1)
                b0             subresi (A, B) = subresi (B, prem(A, B, x)),                0 ≤ i < n − 1.
                                                                                                            

            Remark 1.1. In the integral domain R, b0 may not be invertible. We use
                               −2 (n−i)
            such notations as b0        in the above proof only for the sake of concision.
            This does not do harm to the correctness of the proof. For example,
                                             −2 (n−i)
                     subresi (A, B) = b0                detpol(xn−i B, . . . , B, xn−i−1 R, . . . , R)

            can be viewed as another notation of
                      2 (n−i)
                     b0         subresi (A, B) = detpol(xn−i B, . . . , B, xn−i−1 R, . . . , R).

            So, we use this kind of notations freely in this section.

            Theorem 1.4 (Habicht’s Theorem). Let A and B be as above,

                                        Sn+1 = A, Sn = B, Sn−1 , . . . , S1 , S0

            be the subresultant chain of A and B, and Rn+1 , . . . , R0 the PSC chain.
            Then for each j = 1, . . . , n,
                 2
            (a) Rj+1 Sj−1 = prem(Sj+1 , Sj , x);
                     2 (j−i)
            (b) Rj+1           Si = subresi (Sj+1 , Sj ), 0 ≤ i < j.

            Proof. We use induction on j.
                If j = n, Rn+1 = 1. Therefore, (a) follows by Lemma 1.3, and (b)
            is just the deﬁnition of subresultant. Now, assume the results hold for
            n, n − 1, . . . , j. Then for any i (0 ≤ i < j − 1),
                      −2 (j−i)
            Si = Rj+1            subresi (Sj+1 , Sj )                                   (induction assumption)
                   −2 (j−i) −2 (j−1−i)
                = Rj+1     Rj          subresi (Sj , prem(Sj+1 , Sj , x))               (Lemma 1.3)
                      −2 (j−i)     −2 (j−1−i)                  2
                = Rj+1           Rj             subresi (Sj , Rj+1 Sj−1 )               (induction assumption)
                      −2 (j−1−i)
                = Rj               subresi (Sj , Sj−1 ).

             That means (b) holds for j − 1. Especially, if i = j − 2,

                               Rj2 Sj−2 = subresj−2 (Sj , Sj−1 ) = prem(Sj , Sj−1 , x).

            That completes the proof.                                                                       

May 16, 2016 15:58      ws-book9x6     Automated Inequality Proving and Discovering            mybook    page 12




            12                       Automated Inequality Proving and Discovering


                We then study the relation between the two subresultant chains of two
            polynomials and their images under ring homomorphism. Suppose φ is a
            ring homomorphism mapping R into R̃. Denote also by φ its induced ring
            homomorphism mapping R[x] into R̃[x] such that
                                             
                                              k       k
                                            φ   ai xi =   φ(ai )xi .
                                                i=0             i=0

            Proposition 1.4. Let φ : R → R̃ be a ring homomorphism and φ : R[x] →
            R̃[x] its induced ring homomorphism. Assume f and g are polynomials in
            (1.4) and
                     ã0 = φ(a0 ),    b̃0 = φ(b0 ),    m̃ = deg(φ(f ), x) ≥ ˜l = deg(φ(g), x).
            Deﬁne μ̃ = m̃ − 1 if m̃ > l̃; and μ̃ = m̃ otherwise. Then the image of the
            jth subresultant Sj of f and g under φ is equal to the jth subresultant S̃j
            of φ(f ) and φ(g) multiplied by δ, i.e. φ(Sj ) = δ S̃j , 0 ≤ j < μ̃, where
                            ⎧
                            ⎪
                            ⎪  1,                         if ã0 b̃0 = 0,
                            ⎪
                            ⎪
                            ⎨ ãl−l̃ ,                    if ã0 = 0 and b̃0 = 0,
                                0
                        δ=
                            ⎪
                            ⎪                     m−
                               (−1)(m−m̃) (l−j) b̃0  m̃
                                                        , if ã0 = 0 and b̃0 = 0,
                            ⎪
                            ⎪
                            ⎩
                               0,                         if ã0 = b̃0 = 0.

            Proof. We only prove the third case (l̃ = l) and the other cases are left
            to the readers. If l < j < μ̃, it is obvious that Sj = S̃j = 0. If j ≤ l, we
            have ﬁrst that
                                       Sj (A, B) = (−1)(m−j) (l−j) Sj (B, A).
            Second,
            φ(Sj (B, A)) = detpol(xm−j−1 φ(B), . . . , φ(B), xl−j−1 φ(A), . . . , φ(A))
                             = b̃0m−m̃ detpol(xm̃−j−1 φ(B), . . . , φ(B), xl−j−1 φ(A), . . . , φ(A))
                             = b̃0m−m̃ (−1)(m̃−j) (l−j) ·
                                     detpol(xl−j−1 φ(A), . . . , φ(A), xm̃−j−1 φ(B), . . . , φ(B))
                             = b̃0m−m̃ (−1)(m̃−j) (l−j) subresj (φ(A), φ(B))
                             = (−1)(m̃−j) (l−j) b̃0m−m̃ S̃j .
            Hence
                     φ(Sj ) = (−1)(m+m̃−2 j) (l−j) b̃0m−m̃ S̃j = (−1)(m−m̃) (l−j) b̃m− m̃
                                                                                          S̃j .
                                                                                    0
                                                                                                     

May 16, 2016 15:58    ws-book9x6      Automated Inequality Proving and Discovering                  mybook   page 13




                                           Basics of Elimination Method                                 13


            Remark 1.2. Although we do not deﬁne the subresultant of f and g when
            m < l, the notation Sj (B, A) in the above proof can be easily understood
            as a short notation of detpol(xm−j−1 B, . . . , B, xl−j−1 A, . . . , A).
            Proposition 1.5. Let notations be as above. Assume
                                       φ : Z[a0 , . . . , an+1 , b0 , . . . , bn ] → R̃
            is a ring homomorphism and φ : Z[a0 , . . . , an+1 , b0 , . . . , bn ][x] → R̃[x] is its
            induced ring homomorphism.
                If φ(Sj+1 ) is regular and φ(Sj ) is defective of degree t, then
            (a) φ(Sj−1 ) = φ(Sj−2 ) = · · · = φ(St+1 ) = 0;
            (b) if j = n,
                                                                                   n−t
                                   φ(St ) = [lc(φ(Sn+1 ), x) lc(φ(Sn ), x)]               φ(Sn );
                 if j < n,
                                      φ(Rj+1 )j−t φ(St ) = lc(φ(Sj ), x)j−t φ(Sj );
            (c) if j = n,
                                                                 n−t
                          φ(St−1 ) = [−lc(φ(Sn+1 ), x)]                prem(φ(Sn+1 ), φ(Sn ), x);
                 if j < n,
                             φ(−Rj+1 )j−t+2 φ(St−1 ) = prem(φ(Sj+1 ), φ(Sj ), x).
            Proof.      By the Habicht Theorem, for each i (0 ≤ i < j),
                                            2 (j−i)
                                         Rj+1         Si = subresi (Sj+1 , Sj ).
            Besides, deg(φ(Sj+1 ), x) = j + 1, deg(φ(Sj ), x) = t. Therefore
                      φ(Rj+1 )2 (j−i) φ(Si )
                     = φ(subresi (Sj+1 , Sj ))
                     = φ(lc(Sj+1 , x))j−t subresi (φ(Sj+1 ), φ(Sj ))               (Proposition 1.4)
                                          j−t
                 = lc(φ(Sj+1 ), x) subresi (φ(Sj+1 ), φ(Sj )).
            On the other hand,
                    subresi (φ(Sj+1 ), φ(Sj ))
                            ⎧
                            ⎪
                            ⎨ 0,                                    if t < i < j,
                                          j−t
                         = lc(φ(Sj ), x) φ(Sj ),                    if i = t,
                            ⎪
                            ⎩
                              (−1)j−t+2
                                         prem(φ(Sj+1 ), φ(Sj ), x), if i = t − 1.
            Notice that                               
                                                          lc(φ(Sj+1 ), x), if j < n,
                                     φ(Rj+1 ) =
                                                          1,                if j = n.
            That completes the proof.                                                                   

May 16, 2016 15:58   ws-book9x6      Automated Inequality Proving and Discovering       mybook   page 14




            14                     Automated Inequality Proving and Discovering


            1.3.2    Subresultant Chain Theorem
            The Subresultant Chain Theorem under the case m > l was redeveloped
            by Loos in [Loos (1983)] based on Habicht’s work [Habicht (1948)]. Ho
            and Yap [Ho and Yap (1996)] gave a complete treatment of Loos’ approach
            by introducing the concept of pseudo-subresultants for the case when the
            degrees of the two polynomials are not equal. B. Mishra [Mishra (1993)]
            claimed that the theorem is valid also for the case when the two degrees are
            equal. In this section, we point out that the theorem should be modiﬁed
            when the degrees of the two polynomials are the same.

            Theorem 1.5 (Subresultant Chain m > l). Let
                                                 Sμ+1 , Sμ , . . . , S0
            be the subresultant chain of f (= Sμ+1 ) and g (= Sμ ) in (1.4) with m > l
            and
                                                Rμ+1 , Rμ , . . . , R0 ,
            be its PSC coeﬃcients. If for some j (1 ≤ j ≤ μ), Sj+1 and Sj are both
            regular, then
                                        2
                                       Rj+1 Sj−1 = prem(Sj+1 , Sj , x).
            If Sj+1 is regular but Sj is defective of degree t (< j), then
                                       Sj−1 = Sj−2 = · · · = St+1 = 0,
                                            j−t
                                           Rj+1 St = lc(Sj , x)j−t Sj ,
                                           j−t+2
                                  (−1)j−t Rj+1   St−1 = prem(Sj+1 , Sj , x).

            Theorem 1.6 (Subresultant Chain m = l). Let notations be as above
            but m = l. If Sμ and Sμ−1 are both regular, then
                                        Rμ Sμ−2 = prem(Sμ , Sμ−1 , x).
            If Sμ is regular but Sμ−1 is defective of degree t, then
                                       Sμ−2 = Sμ−3 = · · · = St+1 = 0,
                                         St = lc(Sμ−1 , x)μ−t−1 Sμ−1 ,
                                  (−1)μ−t+1 Rμ St−1 = prem(Sμ , Sμ−1 , x).

                 If for some j (1 ≤ j < μ − 1), Sj+1 and Sj are both regular, then
                              2
                             Rj+1 Sj−1 = prem(Sj+1 , Sj , x),              j < μ − 1.

May 16, 2016 15:58    ws-book9x6      Automated Inequality Proving and Discovering                mybook   page 15




                                           Basics of Elimination Method                               15


            If Sj+1 is regular but Sj is defective of degree t, then
                                         Sj−1 = Sj−2 = · · · = St+1 = 0,
                                              j−t
                                             Rj+1 St = lc(Sj , x)j−t Sj ,
                                  j−t+2
                         (−1)j−t Rj+1   St−1 = prem(Sj+1 , Sj , x),                  j < μ − 1.

                It is easy to see that the relations under the case j < μ − 1 in Theorem
            1.6 do not hold under the case j = μ − 1.
                Consider two polynomials in symbolic coeﬃcients
                           A = c0 xμ+1 + · · · + cμ+1 ,            B = d0 xμ + · · · + dμ .
            Let
                                    ∗
                                   Sμ+1 , Sμ∗ , . . . , S0∗ and Rμ+1
                                                                 ∗
                                                                     , Rμ∗ , . . . , R0∗
            be the subresultant chain and the PSC chain of A and B with respect to x,
            respectively.
               Polynomials f, g can be viewed as the images of A, B under the ring
            homomorphism φ :
                             φ(ci ) = coef(f, xμ+1−i ), 0 ≤ i ≤ μ + 1,
                                   φ(dj ) = coef(g, xμ−j ),            0 ≤ j ≤ μ.
            Proof of Theorem 1.5 We ﬁrst prove the theorem under the case m > l,
            where μ = m − 1 and Sμ+1 is regular. By Proposition 1.4, we have the
            following results:
              • For any i (0 ≤ i < μ),
                      φ(Si∗ ) = φ(subresi (A, B)) = aμ−l                       μ−l
                                                     0 subresi (φ(A), φ(B)) = a0 Si .
              • φ(Si∗ ) is regular if and only if Si is regular; φ(Si∗ ) is defective of degree
                t if and only if Si is defective of degree t.
              • If Si (0 ≤ i < μ) is regular, then
                                   φ(Ri∗ ) = φ(lc(Si∗ , x)) = lc(φ(Si∗ ), x) = aμ−l
                                                                                0 Ri .

                  (1) If Sj+1 and Sj are both regular, by the Habicht Theorem,
                        ∗       ∗           ∗
                      (Rj+1 )2 Sj−1 = prem(Sj+1 , Sj∗ , x) = detpol(xSj∗ , Sj∗ , Sj+1
                                                                                  ∗
                                                                                      ).
               If j = μ or j = μ− 1, then Sμ is regular. By Proposition 1.4, φ(Si∗ ) = Si
            (0 ≤ i ≤ μ + 1), and thus the conclusion follows. If j < μ − 1, then
                             ∗         ∗                2 (μ−l)
                          φ(Rj+1 )2 φ(Sj−1 ) = a0                  2
                                                                  Rj+1 aμ−l
                                                                        0 Sj−1
                                                   = detpol(xφ(Sj∗ ), φ(Sj∗ ), φ(Sj+1
                                                                                  ∗
                                                                                      ))
                                                        3 (μ−l)
                                                   = a0           detpol(xSj , Sj , Sj+1 )
                                                      3 (μ−l)
                                                   = a0       prem(Sj+1 , Sj , x).

May 16, 2016 15:58     ws-book9x6      Automated Inequality Proving and Discovering                  mybook     page 16




            16                       Automated Inequality Proving and Discovering


            That is
                                           2
                                          Rj+1 Sj−1 = prem(Sj+1 , Sj , x).

               (2) If Sj+1 is regular but Sj is defective of degree t.
               (a) If j = μ, i.e. Sμ+1 is regular but Sμ is defective of degree t (= l),
            then by the deﬁnition of subresultant,

                             Sμ−1 = Sμ−2 = · · · = St+1 = 0, and Rμ+1 = 1.

            Therefore
                           μ−t
                          Rμ+1 St = St = lc(Sμ , x)μ+1−t−1 Sμ = lc(Sμ , x)μ−t Sμ .

            Finally,
                     (−Rμ+1 )μ−t+2 St−1 = (−1)μ−t+2 St−1
                                              = (−1)μ−t+2 (−1)μ+1−t+1 prem(Sμ+1 , Sμ , x)
                                              = prem(Sμ+1 , Sμ , x).
                (b) If j = μ− 1, then Sμ is regular. By Proposition 1.4, φ(Si∗ ) = Si (0 ≤
            i ≤ μ + 1). Then the conclusion is clear.
                                          ∗
                (c) If j < μ − 1, i.e. φ(Sj+1 ) is regular but φ(Sj∗ ) is defective of degree
            t, then by Proposition 1.5,
                                       ∗          ∗                  ∗
                                    φ(Sj−1 ) = φ(Sj−2 ) = · · · = φ(St+1 ) = 0,
                                       ∗
                                    φ(Rj+1 )j−t φ(St∗ ) = lc(φ(Sj∗ ), x)j−t φ(Sj∗ ),
                               ∗             ∗               ∗
                           φ(−Rj+1 )j−t+2 φ(St−1 ) = prem(φ(Sj+1 ), φ(Sj∗ ), x).

                 By the above discussion, the above relations give

                                          aμ−l               μ−l
                                           0 Sj−1 = · · · = a0 St+1 = 0,
                              (j−t) (μ−l)    j−t μ−l
                            a0              Rj+1 a0 St = [aμ−l
                                                           0 lc(Sj , x)]
                                                                         j−t μ−l
                                                                            a 0 Sj ,
                        (−aμ−l
                           0 Rj+1 )
                                   j−t+2 μ−l
                                        a0 St−1 = prem(aμ−l       μ−l
                                                        0 Sj+1 , a0 Sj , x)
                                                                         (μ−l) (j−t+2)
                                                              = aμ−l
                                                                 0 a0                    prem(Sj+1 , Sj , x).
            Therefore
                                              Sj−1 = · · · = St+1 = 0,
                                              j−t
                                             Rj+1 St = lc(Sj , x)j−t Sj ,
                                    (−Rj+1 )j−t+2 St−1 = prem(Sj+1 , Sj , x).
                 That completes the proof of Theorem 1.5.

May 16, 2016 15:58     ws-book9x6      Automated Inequality Proving and Discovering           mybook       page 17




                                           Basics of Elimination Method                              17


            Proof of Theorem 1.6 We now prove the theorem under the case m = l,
            where μ = m = l, Sμ+1 is defective of degree m and Sμ is regular. By
            Proposition 1.4, we have the following results:
              • For any i (0 ≤ i < μ),
                          φ(Si∗ ) = φ(subresi (A, B)) = (−1)μ−i b0 subresi (φ(A), φ(B))
                                    = (−1)μ−i b0 Si .
              • φ(Si∗ ) is regular if and only if Si is regular; φ(Si∗ ) is defective of degree
                t if and only if Si is defective of degree t.
              • If Si (0 ≤ i < μ) is regular, then
                                                 φ(Ri∗ ) = (−1)μ−i b0 Ri .
                (1) Assume Sj+1 and Sj are both regular. By the Habicht Theorem,
                         ∗       ∗           ∗
                       (Rj+1 )2 Sj−1 = prem(Sj+1 , Sj∗ , x) = detpol(xSj∗ , Sj∗ , Sj+1
                                                                                   ∗
                                                                                       ).
                (a) If j = μ − 1,
                       (Rμ∗ )2 Sμ−2
                                ∗
                                    = prem(Sμ∗ , Sμ−1
                                                  ∗                   ∗
                                                      , x) = detpol(xSμ−1    ∗
                                                                          , Sμ−1 , Sμ∗ ).
            Hence
                 φ((Rμ∗ )2 Sμ−2
                            ∗
                                ) = Rμ2 · (−1)2 · b0 · Sμ−2
                                        = detpol(x · (−1) · b0 · Sμ−1 , (−1) · b0 · Sμ−1 , Sμ )
                                        = b20 prem(Sμ , Sμ−1 , x).
            Because Rμ = b0 ,
                                          Rμ Sμ−2 = prem(Sμ , Sμ−1 , x).
                (b) If j < μ − 1,
             (−1)2 (μ−j−1) b20 · Rj+1
                                  2
                                      · (−1)μ−j+1 b0 · Sj−1
                     = detpol(x · (−1)μ−j · b0 · Sj , (−1)μ−j · b0 · Sj , (−1)μ−j−1 · b0 · Sj+1 )
                     = (−1)3 (μ−1)−1 b30 detpol(xSj , Sj , Sj+1 )
                     = (−1)3 (μ−1)−1 b30 prem(Sj+1 , Sj , x).
            Hence
                                          2
                                         Rj+1 Sj−1 = prem(Sj+1 , Sj , x).
                                                                                    ∗
                (2) Assume Sj+1 is regular but Sj is defective of degree t, i.e. φ(Sj+1 )
                              ∗
            is regular but φ(Sj ) is defective of degree t. By Proposition 1.5,
                                       ∗          ∗                  ∗
                                    φ(Sj−1 ) = φ(Sj−2 ) = · · · = φ(St+1 ) = 0,                    (1.9)
                                   ∗
                               φ(Rj+1  )j−t φ(St∗ ) = lc(φ(Sj∗ ), x)j−t φ(Sj∗ ),                  (1.10)
                                ∗              ∗                    ∗
                            φ(−Rj+1 )j−t+2 φ(St−1   ) = prem(φ(Sj+1    ), φ(Sj∗ ), x).            (1.11)

May 16, 2016 15:58     ws-book9x6     Automated Inequality Proving and Discovering            mybook   page 18




            18                      Automated Inequality Proving and Discovering


                  Obviously, formula (1.9) implies Sj−1 = Sj−2 = · · · = St+1 = 0.
                  (a) If j = μ − 1, formula (1.10) becomes
                 Rμμ−t−1 · (−1)μ−t b0 · St = (−b0 )μ−t−1 · lc(Sμ−1 , x)μ−t−1 · (−b0 ) · Sμ−1 .
            Note that Rμ = b0 , which implies
                                          St = lc(Sμ−1 , x)μ−t−1 Sμ−1 .
            Besides, formula (1.11) becomes
              (−1)μ−t+1 Rμμ−t+1 · (−1)μ−t+1 b0 · St−1 = prem(Sμ , −b0 Sμ−1 , x)
                                                                 = (−b0 )μ−t+1 prem(Sμ , Sμ−1 , x).
            Therefore (−1)μ−t+1 Rμ St−1 = prem(Sμ , Sμ−1 , x).
              (b) If j < μ − 1, it can be veriﬁed similarly that
                                             j−t
                                            Rj+1 St = lc(Sj , x)j−t Sj ,
                                    (−Rj+1 )j−t+2 St−1 = prem(Sj+1 , Sj , x).
               That completes the proof of Theorem 1.6.
               Theorems 1.5 and 1.6 provide an eﬀective algorithm for computing sub-
            resultant chain via pseudo-division. If deg(Sμ+1 , x) = deg(Sμ , x), Sμ+1 is
            defective and thus the theorems do not give a way for computing Sμ−1 .
            However, by Proposition 1.3,
                                          Sμ−1 = −prem(Sμ+1 , Sμ , x).
                By Theorems 1.5 and 1.6 and the discussion above, we may give the
            following algorithm (Algorithm 1.1) for computing the subresultant chain
            of two polynomials.

            Notation 1.1. For a list (or set) L = [a1 , ..., an ], let op(L) denote a1 , ..., an .
            Then, when we describe an algorithm, L ← [op(L), b] means appending b to
            the list L.


            1.3.3      Subresultant Polynomial Remainder Sequence

            Deﬁnition 1.6. Let p1 and p2 be two polynomials in R[x] with deg(p1 ) ≥
            deg(p2 ). A sequence of nonzero polynomials p1 , p2 , . . . , pt in R[x] is called
            the subresultant polynomial remainder sequence of p1 and p2 with respect
            to x, if
                              pi+2 = prem(pi , pi+1 , x)/βi+2 ,          1 ≤ i ≤ t − 2,
                                        prem(pt−1 , pt , x) = 0,

May 16, 2016 15:58   ws-book9x6    Automated Inequality Proving and Discovering                      mybook   page 19




                                       Basics of Elimination Method                                      19


            Algorithm 1.1 SubRes
            Input: Two polynomials f, g ∈ R[x] with deg(f ) ≥ deg(g) > 0
            Output: The subresultant chain of f and g with respect to x
             1: m ← deg(f ); l ← deg(g);
             2: if m > l then j ← m − 1 else j ← l endif ;
             3: Sj+1 ← f ; Sj ← g; Rj+1 ← 1; μ ← j; L ← [f, g];
             4: if m = l then
             5:    Sj−1 ← − prem(Sj+1 , Sj , x);
             6:    L ← [op(L), Sj−1 ]; j ← j − 1; Rj+1 ← lc(Sj+1 , x);
             7: end if
             8: do
             9:    if Sj = 0 then r ← − 1 else r ← deg(Sj ) endif ;
            10:    for i from r + 1 to j − 1 do Si ← 0; L ← [op(L), 0] enddo;
            11:    if 0 <= r and r < j then
            12:       if m = l and j = μ − 1 then
            13:          Sr ← lc(Sj , x)j−r Sj ;
            14:          Sr−1 ← prem(Sj+1 , Sj , x)/((−1)j−r Rj+1 );
            15:       else
                                                  j−r
            16:          Sr ← lc(Sj , x)j−r Sj /(Rj+1 );
            17:          Sr−1 ← prem(Sj+1 , Sj , x)/((−Rj+1 )j−r+2 );
            18:       endif ;
            19:       if r > 0 then L ← [op(L), Sr , Sr−1 ] else L ← [op(L), Sr ] endif ;
            20:    elseif 0 < r and r = j then
            21:       if m = l and j = μ − 1 then
            22:          Sr−1 ← prem(Sj+1 , Sj , x)/Rj+1
            23:       else
            24:          Sr−1 ← prem(Sj+1 , Sj , x)/Rj+1 2

            25:       endif ;
            26:       L ← [op(L), Sr−1 ]
            27:    endif ;
            28:    if r <= 0 then return L
            29:    else     j ← r − 1; Rj+1 ← lc(Sj+1 , x);
            30:    endif ;
            31: enddo


            where
                                                   δi −1
                     β3 = (−1)δ2 , βi+1 = (−1)δi ψi−1    Ii−1 ,              i = 3, . . . , t − 1,
                     I1 = 1, Ii = lc(pi , x),                                i = 2, . . . , t,
                     δi = di−1 − di + 1,                                     i = 2, . . . , t,
                     di = deg(pi , x),                                       i = 1, . . . , t,
                                                               δi −1
                                                           Ii
                     ψ1 = 1, ψ2 = I2δ2 −1 , ψi = ψi−1                  ,     i = 3, . . . , t.
                                                         ψi−1

May 16, 2016 15:58   ws-book9x6     Automated Inequality Proving and Discovering         mybook   page 20




            20                    Automated Inequality Proving and Discovering


                We establish the relation between the subresultant polynomial remain-
            der sequence and the subresultant chain, which guarantees that subre-
            sultant polynomial remainder sequence is well-deﬁned. That is to say,
            pi ∈ R[x] for i ≥ 3 whenever p1 , p2 ∈ R[x].
                The subresultant chain theorem shows that the subrestltant chains have
            the so-called “block structure”.

            Deﬁnition 1.7. Let f and g be as above with m = deg(f, x) ≥ deg(g, x) =
            l > 0 and let
                                                S : Sμ+1 , Sμ , . . . , S0
            be the subresultant chain of f and g with respect to x. A sequence
                                                    d1 , d2 , . . . , dt
            of steadily decreasing nonnegative integers is called the block indices of S,
            if d1 = μ + 1, for each 2 ≤ i ≤ t, Sdi is regular, and for 0 ≤ j ≤ μ,
            j ∈ {d2 , . . . , dt }, Sj is defective.
                The sequence of regular subresultants Sd2 , . . . , Sdt is called the subre-
            sultant regular subchain of f and g with respect to x.

               The interesting block structure of S can be described as follows. The
            ﬁrst block consists of the single term Sμ+1 . For any i(2 ≤ i ≤ t),
                      Sdi = 0, Sdi ∼ Sdi−1 −1 , and Sdi−1 −2 = · · · = Sdi +1 = 0.
            So, we say that the ith nonzero block of S has the form
                                             Sdi−1 −1 , 0, . . . , 0, Sdi ,
            where Sdi−1 −1 ∼ Sdi , and di−1 − 1 ≥ di . If dt > 0, then
                                            Sdt −1 = · · · = S0 = 0;
            This is the last block, called the zero block of S.

            Proposition 1.6. Let f and g be as in (1.4) and m ≥ l > 0. Suppose
            S : Sμ+1 , Sμ , . . . , S0 is the subresultant chain of f and g with respect to x
            and d1 , d2 , . . . , dt is the block indices of S.
            (a) If m > l, for i = 1, . . . , t − 1,
                                           −2
                                                Sdi+1 = lc(Sdi −1 , x)δi+1 −2 Sdi −1 ,
                                     δ
                                   Rdi+1
                                     i
                                                                                         (1.12)
                                           δi+1
                                  (−Rdi )         Sdi+1 −1 = prem(Sdi , Sdi −1 , x),     (1.13)
                 where δi+1 = di − di+1 + 1.

May 16, 2016 15:58   ws-book9x6     Automated Inequality Proving and Discovering             mybook     page 21




                                        Basics of Elimination Method                              21


            (b) If m = l, setting
                                     δ2 = 1,     δi+1 = di − di+1 + 1, i ≥ 2,
                 we have that Eq. (1.12) and Eq. (1.13) hold for i = 3, . . . , t − 1 and
                                  Sd2 = Sd1 −1 ,     Sd3 = lc(Sd2 −1 , x)δ3 −2 Sd2 −1 ,
                                         −Sd2 −1 = prem(Sd1 , Sd1 −1 , x),                    (1.14)
                                          δ3
                                    (−1) Rd2 Sd3 −1 = prem(Sd2 , Sd2 −1 , x).

            Proof. (a) m > l. If di − 1 = di+1 , then δi+1 − 2 = 0. So Eq. (1.12) is an
            identity. Equation (1.13) can be obtained by taking j + 1 = di in Theorem
            1.5.
                If di − 1 > di+1 , taking
                                         Sj+1 = Sdi and St = Sdi+1
            in Theorem 1.5, we have
                                       Sj = Sdi −1 ,     St−1 = Sdi+1 −1 .
            On the other hand,
                            Rj+1 = Rdi ,       j − t = di − di+1 − 1 = δi+1 − 2.
            The conclusion follows.
               (b) m = l. We only need to notice that d1 = μ + 1, d2 = μ. Then
            the conclusion (1.14) is either obvious or can be obtained directly from
            Theorem 1.6 by letting j = μ − 1. The proof for the case i > 2 is similar to
            that for m > l.                                                           
                The above proposition shows that
                            Sdi+1 ∼ Sdi −1 ,       Sdi+1 −1 ∼ prem(Sdi , Sdi −1 , x).
            Hence
                          Sdi+2 ∼ prem(Sdi , Sdi −1 , x) ∼ prem(Sdi , Sdi+1 , x).
            Under the assumption of Proposition 1.6, if p1 = f, p2 = g, . . . , pk is a
            polynomial remainder sequence of f and g, then k = t and Sdi ∼ pi ,
            i = 1, . . . , t.

            Theorem 1.7. Let f, g ∈ R[x] be as in (1.4) and m ≥ l > 0. Suppose
            S : Sμ+1 , Sμ , . . . , S0 is the subresultant chain of f and g with respect to x,
            d1 , d2 , . . . , dt is the block indices of S and p1 = f, p2 = g, p3 , . . . , pt is the
            subresultant polynomial remainder sequence of f and g with respect to x.
            Then we have

May 16, 2016 15:58   ws-book9x6     Automated Inequality Proving and Discovering         mybook   page 22




            22                    Automated Inequality Proving and Discovering


            (a) pi = Sdi−1 −1 for i = 1, . . . , t, where d0 − 1 = μ + 1. In other words,

                                            Sd0 −1 , Sd1 −1 , . . . , Sdt−1 −1

                is the subresultant polynomial remainder sequence of f and g with re-
                spect to x.
            (b) ψi = Rdi for i = 1, . . . , t with only one exception that ψ2 = 1 = Rd2
                when m = l.

            Proof. Because Sdi ∼ pi (1 ≤ i ≤ t), the deﬁnition of δi = di−1 − di + 1
            in Deﬁnition 1.6 is consistent with the above notation of δi .
               We use induction on i. If i = 1, by the deﬁnition,

                      p1 = f = Sμ+1 = Sd1 = Sd0 −1 ,             ψ1 = 1 = Rμ+1 = Rd1 .

            If i = 2, p2 = Sμ = Sd1 −1 . When m > l, taking i = 1 in formula (1.12), we
            have

                                              Rd2 = I2δ2 −1 = ψ2 .

            When m = l, we have δ2 = 1 and thus ψ2 = 1.
              If i = 3, by Proposition 1.6,

                                      prem(p1 , p2 , x) = (−1)δ2 Sd2 −1 .

            The above relation holds for either m > l or m = l. So, p3 = Sd2 −1 . When
            m > l, setting i = 2 in formula (1.12), we have ψ3 = Rd3 . When m = l,
            since ψ2 = 1 and

                                       Sd3 = lc(Sd2 −1 , x)δ3 −2 Sd2 −1 ,

            we still have ψ3 = Rd3 .
               If i = 4, β4 = (−1)δ3 · ψ2δ3 −1 · lc(Sd1 −1 , x). When m > l, we have

                                    β4 = (−1)δ3 · Rdδ32−1 · lc(Sd1 −1 , x).

            When m = l, we have

                                              β4 = (−1)δ3 · Rd2 .

            Hence, we have p4 = Sd3 −1 when m ≥ l.
               Now we assume the two conclusions of the theorem hold respectively
            when j ≤ i and j ≤ i − 1 (i ≥ 4). Then by formula (1.12),
                                           I δi −1
                                               i
                                    Rdi =              Rdi−1 .
                                            Rdi−1

May 16, 2016 15:58     ws-book9x6   Automated Inequality Proving and Discovering                mybook   page 23




                                        Basics of Elimination Method                                23


            First, by the induction assumption, it is clear that ψi = Rdi . Second,
                     prem(pi−1 , pi , x) = prem(Sdi−2 −1 , Sdi−1 −1 , x)
                                                                                         
                                                    Rdi−2 δi−1 −2
                                         = prem                       · Sdi−1 , Sdi−1 −1 , x
                                                     Ii−1
                                                  
                                             Rdi−2 δi−1 −2
                                         =                  · prem(Sdi−1 , Sdi−1 −1 , x)
                                              Ii−1
                                                  
                                             Rdi−2 δi−1 −2
                                         =                  · (−Rdi−1 )δi · Sdi −1
                                              Ii−1
                                                      −1
                                        = (−1)δi Rdδii−1 Ii−1 Sdi −1
                                                  δi −1
                                        = (−1)δi ψi−1   Ii−1 Sdi −1
                                        = βi+1 Sdi −1 .
            That completes the proof.                                                               
