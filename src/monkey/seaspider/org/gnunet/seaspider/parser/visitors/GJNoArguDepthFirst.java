//
// Generated by JTB 1.3.2
//

package org.gnunet.seaspider.parser.visitors;
import org.gnunet.seaspider.parser.nodes.*;

import java.util.*;

/**
 * Provides default methods which visit each node in the tree in depth-first
 * order.  Your visitors may extend this class.
 */
public class GJNoArguDepthFirst<R> implements GJNoArguVisitor<R> {
   //
   // Auto class visitors--probably don't need to be overridden.
   //
   public R visit(NodeList n) {
      R _ret=null;
      int _count=0;
      for ( Enumeration<Node> e = n.elements(); e.hasMoreElements(); ) {
         e.nextElement().accept(this);
         _count++;
      }
      return _ret;
   }

   public R visit(NodeListOptional n) {
      if ( n.present() ) {
         R _ret=null;
         int _count=0;
         for ( Enumeration<Node> e = n.elements(); e.hasMoreElements(); ) {
            e.nextElement().accept(this);
            _count++;
         }
         return _ret;
      }
      else
         return null;
   }

   public R visit(NodeOptional n) {
      if ( n.present() )
         return n.node.accept(this);
      else
         return null;
   }

   public R visit(NodeSequence n) {
      R _ret=null;
      int _count=0;
      for ( Enumeration<Node> e = n.elements(); e.hasMoreElements(); ) {
         e.nextElement().accept(this);
         _count++;
      }
      return _ret;
   }

   public R visit(NodeToken n) { return null; }

   //
   // User-generated visitor methods below
   //

   /**
    * <PRE>
    * f0 -> ( ExternalDeclaration() )+
    * </PRE>
    */
   public R visit(TranslationUnit n) {
      R _ret=null;
      n.f0.accept(this);
      return _ret;
   }

   /**
    * <PRE>
    * f0 -> ( StorageClassSpecifier() )*
    * f1 -> ( FunctionDeclaration() | StructOrUnionSpecifier() | VariableDeclaration() | TypeDeclaration() )
    * </PRE>
    */
   public R visit(ExternalDeclaration n) {
      R _ret=null;
      n.f0.accept(this);
      n.f1.accept(this);
      return _ret;
   }

   /**
    * <PRE>
    * f0 -> TypeSpecifier()
    * f1 -> &lt;IDENTIFIER&gt;
    * f2 -> "("
    * f3 -> [ ParameterList() ]
    * f4 -> ")"
    * f5 -> ( ";" | CompoundStatement() )
    * </PRE>
    */
   public R visit(FunctionDeclaration n) {
      R _ret=null;
      n.f0.accept(this);
      n.f1.accept(this);
      n.f2.accept(this);
      n.f3.accept(this);
      n.f4.accept(this);
      n.f5.accept(this);
      return _ret;
   }

   /**
    * <PRE>
    * f0 -> ( &lt;STATIC&gt; | &lt;EXTERN&gt; )
    * </PRE>
    */
   public R visit(StorageClassSpecifier n) {
      R _ret=null;
      n.f0.accept(this);
      return _ret;
   }

   /**
    * <PRE>
    * f0 -> &lt;TYPEDEF&gt;
    * f1 -> ( DataType() | FunctionType() )
    * f2 -> ";"
    * </PRE>
    */
   public R visit(TypeDeclaration n) {
      R _ret=null;
      n.f0.accept(this);
      n.f1.accept(this);
      n.f2.accept(this);
      return _ret;
   }

   /**
    * <PRE>
    * f0 -> StructOrUnionSpecifier()
    * f1 -> &lt;IDENTIFIER&gt;
    * </PRE>
    */
   public R visit(DataType n) {
      R _ret=null;
      n.f0.accept(this);
      n.f1.accept(this);
      return _ret;
   }

   /**
    * <PRE>
    * f0 -> TypeSpecifier()
    * f1 -> "("
    * f2 -> "*"
    * f3 -> &lt;IDENTIFIER&gt;
    * f4 -> ")"
    * f5 -> "("
    * f6 -> [ ParameterList() ]
    * f7 -> ")"
    * </PRE>
    */
   public R visit(FunctionType n) {
      R _ret=null;
      n.f0.accept(this);
      n.f1.accept(this);
      n.f2.accept(this);
      n.f3.accept(this);
      n.f4.accept(this);
      n.f5.accept(this);
      n.f6.accept(this);
      n.f7.accept(this);
      return _ret;
   }

   /**
    * <PRE>
    * f0 -> ParameterDeclaration()
    * f1 -> ( "," ParameterDeclaration() )*
    * f2 -> [ "," "..." ]
    * </PRE>
    */
   public R visit(ParameterList n) {
      R _ret=null;
      n.f0.accept(this);
      n.f1.accept(this);
      n.f2.accept(this);
      return _ret;
   }

   /**
    * <PRE>
    * f0 -> TypeSpecifier()
    * f1 -> &lt;IDENTIFIER&gt;
    * f2 -> [ Array() ]
    * </PRE>
    */
   public R visit(ParameterDeclaration n) {
      R _ret=null;
      n.f0.accept(this);
      n.f1.accept(this);
      n.f2.accept(this);
      return _ret;
   }

   /**
    * <PRE>
    * f0 -> VariableClassSpecifier()
    * f1 -> TypeSpecifier()
    * f2 -> InitDeclaratorList()
    * f3 -> ";"
    * </PRE>
    */
   public R visit(VariableDeclaration n) {
      R _ret=null;
      n.f0.accept(this);
      n.f1.accept(this);
      n.f2.accept(this);
      n.f3.accept(this);
      return _ret;
   }

   /**
    * <PRE>
    * f0 -> [ &lt;STATIC&gt; ]
    * f1 -> VariableDeclaration()
    * </PRE>
    */
   public R visit(LocalVariableDeclaration n) {
      R _ret=null;
      n.f0.accept(this);
      n.f1.accept(this);
      return _ret;
   }

   /**
    * <PRE>
    * f0 -> ( &lt;AUTO&gt; | &lt;REGISTER&gt; )*
    * </PRE>
    */
   public R visit(VariableClassSpecifier n) {
      R _ret=null;
      n.f0.accept(this);
      return _ret;
   }

   /**
    * <PRE>
    * f0 -> [ &lt;CONST&gt; ]
    * f1 -> ( &lt;VOID&gt; | &lt;CHAR&gt; | &lt;SHORT&gt; [ &lt;INT&gt; ] | &lt;INT&gt; | &lt;LONG&gt; [ &lt;LONG&gt; ] | &lt;FLOAT&gt; | &lt;DOUBLE&gt; | ( &lt;SIGNED&gt; | &lt;UNSIGNED&gt; ) [ &lt;CHAR&gt; | &lt;SHORT&gt; [ &lt;INT&gt; ] | &lt;INT&gt; | &lt;LONG&gt; [ &lt;LONG&gt; ] ] | StructOrUnionSpecifier() | EnumSpecifier() | &lt;IDENTIFIER&gt; )
    * f2 -> [ Pointer() ]
    * f3 -> [ Array() ]
    * </PRE>
    */
   public R visit(TypeSpecifier n) {
      R _ret=null;
      n.f0.accept(this);
      n.f1.accept(this);
      n.f2.accept(this);
      n.f3.accept(this);
      return _ret;
   }

   /**
    * <PRE>
    * f0 -> [ &lt;CONST&gt; ]
    * f1 -> ( &lt;VOID&gt; | &lt;CHAR&gt; | &lt;SHORT&gt; [ &lt;INT&gt; ] | &lt;INT&gt; | &lt;LONG&gt; [ &lt;LONG&gt; ] | &lt;FLOAT&gt; | &lt;DOUBLE&gt; | ( &lt;SIGNED&gt; | &lt;UNSIGNED&gt; ) [ &lt;CHAR&gt; | &lt;SHORT&gt; [ &lt;INT&gt; ] | &lt;INT&gt; | &lt;LONG&gt; [ &lt;LONG&gt; ] ] | StructOrUnionSpecifier() | EnumSpecifier() )
    * f2 -> [ Pointer() ]
    * f3 -> [ Array() ]
    * </PRE>
    */
   public R visit(NoIdentifierTypeSpecifier n) {
      R _ret=null;
      n.f0.accept(this);
      n.f1.accept(this);
      n.f2.accept(this);
      n.f3.accept(this);
      return _ret;
   }

   /**
    * <PRE>
    * f0 -> StructOrUnion() [ &lt;IDENTIFIER&gt; ] "{" StructDeclarationList() "}"
    *       | StructOrUnion() &lt;IDENTIFIER&gt;
    * </PRE>
    */
   public R visit(StructOrUnionSpecifier n) {
      R _ret=null;
      n.f0.accept(this);
      return _ret;
   }

   /**
    * <PRE>
    * f0 -> ( &lt;STRUCT&gt; | &lt;UNION&gt; )
    * </PRE>
    */
   public R visit(StructOrUnion n) {
      R _ret=null;
      n.f0.accept(this);
      return _ret;
   }

   /**
    * <PRE>
    * f0 -> ( StructDeclaration() )+
    * </PRE>
    */
   public R visit(StructDeclarationList n) {
      R _ret=null;
      n.f0.accept(this);
      return _ret;
   }

   /**
    * <PRE>
    * f0 -> InitDeclarator()
    * f1 -> ( "," InitDeclarator() )*
    * </PRE>
    */
   public R visit(InitDeclaratorList n) {
      R _ret=null;
      n.f0.accept(this);
      n.f1.accept(this);
      return _ret;
   }

   /**
    * <PRE>
    * f0 -> &lt;IDENTIFIER&gt;
    * f1 -> [ Array() ]
    * f2 -> [ "=" Initializer() ]
    * </PRE>
    */
   public R visit(InitDeclarator n) {
      R _ret=null;
      n.f0.accept(this);
      n.f1.accept(this);
      n.f2.accept(this);
      return _ret;
   }

   /**
    * <PRE>
    * f0 -> TypeSpecifier()
    * f1 -> &lt;IDENTIFIER&gt;
    * f2 -> [ Array() | ":" ConstantExpression() ]
    * f3 -> [ &lt;IDENTIFIER&gt; ]
    * f4 -> ";"
    * </PRE>
    */
   public R visit(StructDeclaration n) {
      R _ret=null;
      n.f0.accept(this);
      n.f1.accept(this);
      n.f2.accept(this);
      n.f3.accept(this);
      n.f4.accept(this);
      return _ret;
   }

   /**
    * <PRE>
    * f0 -> &lt;ENUM&gt;
    * f1 -> ( [ &lt;IDENTIFIER&gt; ] "{" EnumeratorList() "}" | &lt;IDENTIFIER&gt; )
    * </PRE>
    */
   public R visit(EnumSpecifier n) {
      R _ret=null;
      n.f0.accept(this);
      n.f1.accept(this);
      return _ret;
   }

   /**
    * <PRE>
    * f0 -> Enumerator()
    * f1 -> ( "," Enumerator() )*
    * </PRE>
    */
   public R visit(EnumeratorList n) {
      R _ret=null;
      n.f0.accept(this);
      n.f1.accept(this);
      return _ret;
   }

   /**
    * <PRE>
    * f0 -> &lt;IDENTIFIER&gt;
    * f1 -> [ "=" ConstantExpression() ]
    * </PRE>
    */
   public R visit(Enumerator n) {
      R _ret=null;
      n.f0.accept(this);
      n.f1.accept(this);
      return _ret;
   }

   /**
    * <PRE>
    * f0 -> "*"
    * f1 -> [ &lt;CONST&gt; ]
    * f2 -> [ Pointer() ]
    * </PRE>
    */
   public R visit(Pointer n) {
      R _ret=null;
      n.f0.accept(this);
      n.f1.accept(this);
      n.f2.accept(this);
      return _ret;
   }

   /**
    * <PRE>
    * f0 -> &lt;IDENTIFIER&gt;
    * f1 -> ( "," &lt;IDENTIFIER&gt; )*
    * </PRE>
    */
   public R visit(IdentifierList n) {
      R _ret=null;
      n.f0.accept(this);
      n.f1.accept(this);
      return _ret;
   }

   /**
    * <PRE>
    * f0 -> ( AssignmentExpression() | "{" InitializerList() [ "," ] "}" )
    * </PRE>
    */
   public R visit(Initializer n) {
      R _ret=null;
      n.f0.accept(this);
      return _ret;
   }

   /**
    * <PRE>
    * f0 -> Initializer()
    * f1 -> ( "," Initializer() )*
    * </PRE>
    */
   public R visit(InitializerList n) {
      R _ret=null;
      n.f0.accept(this);
      n.f1.accept(this);
      return _ret;
   }

   /**
    * <PRE>
    * f0 -> "["
    * f1 -> [ ConstantExpression() ]
    * f2 -> "]"
    * </PRE>
    */
   public R visit(Array n) {
      R _ret=null;
      n.f0.accept(this);
      n.f1.accept(this);
      n.f2.accept(this);
      return _ret;
   }

   /**
    * <PRE>
    * f0 -> ( LabeledStatement() | ExpressionStatement() | CompoundStatement() | SelectionStatement() | IterationStatement() | JumpStatement() )
    * </PRE>
    */
   public R visit(Statement n) {
      R _ret=null;
      n.f0.accept(this);
      return _ret;
   }

   /**
    * <PRE>
    * f0 -> ( &lt;IDENTIFIER&gt; ":" Statement() | &lt;CASE&gt; ConstantExpression() ":" Statement() | &lt;DFLT&gt; ":" Statement() )
    * </PRE>
    */
   public R visit(LabeledStatement n) {
      R _ret=null;
      n.f0.accept(this);
      return _ret;
   }

   /**
    * <PRE>
    * f0 -> [ Expression() ]
    * f1 -> ";"
    * </PRE>
    */
   public R visit(ExpressionStatement n) {
      R _ret=null;
      n.f0.accept(this);
      n.f1.accept(this);
      return _ret;
   }

   /**
    * <PRE>
    * f0 -> "{"
    * f1 -> ( LocalVariableDeclaration() | Statement() )*
    * f2 -> "}"
    * </PRE>
    */
   public R visit(CompoundStatement n) {
      R _ret=null;
      n.f0.accept(this);
      n.f1.accept(this);
      n.f2.accept(this);
      return _ret;
   }

   /**
    * <PRE>
    * f0 -> ( &lt;IF&gt; "(" Expression() ")" Statement() [ &lt;ELSE&gt; Statement() ] | &lt;SWITCH&gt; "(" Expression() ")" Statement() )
    * </PRE>
    */
   public R visit(SelectionStatement n) {
      R _ret=null;
      n.f0.accept(this);
      return _ret;
   }

   /**
    * <PRE>
    * f0 -> ( &lt;WHILE&gt; "(" Expression() ")" Statement() | &lt;DO&gt; Statement() &lt;WHILE&gt; "(" Expression() ")" ";" | &lt;FOR&gt; "(" [ Expression() ] ";" [ Expression() ] ";" [ Expression() ] ")" Statement() )
    * </PRE>
    */
   public R visit(IterationStatement n) {
      R _ret=null;
      n.f0.accept(this);
      return _ret;
   }

   /**
    * <PRE>
    * f0 -> ( &lt;GOTO&gt; &lt;IDENTIFIER&gt; ";" | &lt;CONTINUE&gt; ";" | &lt;BREAK&gt; ";" | &lt;RETURN&gt; [ Expression() ] ";" )
    * </PRE>
    */
   public R visit(JumpStatement n) {
      R _ret=null;
      n.f0.accept(this);
      return _ret;
   }

   /**
    * <PRE>
    * f0 -> AssignmentExpression()
    * f1 -> ( "," AssignmentExpression() )*
    * </PRE>
    */
   public R visit(Expression n) {
      R _ret=null;
      n.f0.accept(this);
      n.f1.accept(this);
      return _ret;
   }

   /**
    * <PRE>
    * f0 -> UnaryExpression() AssignmentOperator() AssignmentExpression()
    *       | ConditionalExpression()
    * </PRE>
    */
   public R visit(AssignmentExpression n) {
      R _ret=null;
      n.f0.accept(this);
      return _ret;
   }

   /**
    * <PRE>
    * f0 -> ( "=" | "*=" | "/=" | "%=" | "+=" | "-=" | "&lt;&lt;=" | "&gt;&gt;=" | "&=" | "^=" | "|=" )
    * </PRE>
    */
   public R visit(AssignmentOperator n) {
      R _ret=null;
      n.f0.accept(this);
      return _ret;
   }

   /**
    * <PRE>
    * f0 -> LogicalORExpression()
    * f1 -> [ "?" Expression() ":" ConditionalExpression() ]
    * </PRE>
    */
   public R visit(ConditionalExpression n) {
      R _ret=null;
      n.f0.accept(this);
      n.f1.accept(this);
      return _ret;
   }

   /**
    * <PRE>
    * f0 -> ConditionalExpression()
    * </PRE>
    */
   public R visit(ConstantExpression n) {
      R _ret=null;
      n.f0.accept(this);
      return _ret;
   }

   /**
    * <PRE>
    * f0 -> LogicalANDExpression()
    * f1 -> [ "||" LogicalORExpression() ]
    * </PRE>
    */
   public R visit(LogicalORExpression n) {
      R _ret=null;
      n.f0.accept(this);
      n.f1.accept(this);
      return _ret;
   }

   /**
    * <PRE>
    * f0 -> InclusiveORExpression()
    * f1 -> [ "&&" LogicalANDExpression() ]
    * </PRE>
    */
   public R visit(LogicalANDExpression n) {
      R _ret=null;
      n.f0.accept(this);
      n.f1.accept(this);
      return _ret;
   }

   /**
    * <PRE>
    * f0 -> ExclusiveORExpression()
    * f1 -> [ "|" InclusiveORExpression() ]
    * </PRE>
    */
   public R visit(InclusiveORExpression n) {
      R _ret=null;
      n.f0.accept(this);
      n.f1.accept(this);
      return _ret;
   }

   /**
    * <PRE>
    * f0 -> ANDExpression()
    * f1 -> [ "^" ExclusiveORExpression() ]
    * </PRE>
    */
   public R visit(ExclusiveORExpression n) {
      R _ret=null;
      n.f0.accept(this);
      n.f1.accept(this);
      return _ret;
   }

   /**
    * <PRE>
    * f0 -> EqualityExpression()
    * f1 -> [ "&" ANDExpression() ]
    * </PRE>
    */
   public R visit(ANDExpression n) {
      R _ret=null;
      n.f0.accept(this);
      n.f1.accept(this);
      return _ret;
   }

   /**
    * <PRE>
    * f0 -> RelationalExpression()
    * f1 -> [ ( "==" | "!=" ) EqualityExpression() ]
    * </PRE>
    */
   public R visit(EqualityExpression n) {
      R _ret=null;
      n.f0.accept(this);
      n.f1.accept(this);
      return _ret;
   }

   /**
    * <PRE>
    * f0 -> ShiftExpression()
    * f1 -> [ ( "&lt;" | "&gt;" | "&lt;=" | "&gt;=" ) RelationalExpression() ]
    * </PRE>
    */
   public R visit(RelationalExpression n) {
      R _ret=null;
      n.f0.accept(this);
      n.f1.accept(this);
      return _ret;
   }

   /**
    * <PRE>
    * f0 -> AdditiveExpression()
    * f1 -> [ ( "&lt;&lt;" | "&gt;&gt;" ) ShiftExpression() ]
    * </PRE>
    */
   public R visit(ShiftExpression n) {
      R _ret=null;
      n.f0.accept(this);
      n.f1.accept(this);
      return _ret;
   }

   /**
    * <PRE>
    * f0 -> MultiplicativeExpression()
    * f1 -> [ ( "+" | "-" ) AdditiveExpression() ]
    * </PRE>
    */
   public R visit(AdditiveExpression n) {
      R _ret=null;
      n.f0.accept(this);
      n.f1.accept(this);
      return _ret;
   }

   /**
    * <PRE>
    * f0 -> CastExpression()
    * f1 -> [ ( "*" | "/" | "%" ) MultiplicativeExpression() ]
    * </PRE>
    */
   public R visit(MultiplicativeExpression n) {
      R _ret=null;
      n.f0.accept(this);
      n.f1.accept(this);
      return _ret;
   }

   /**
    * <PRE>
    * f0 -> ( "(" TypeSpecifier() ")" CastExpression() | UnaryExpression() )
    * </PRE>
    */
   public R visit(CastExpression n) {
      R _ret=null;
      n.f0.accept(this);
      return _ret;
   }

   /**
    * <PRE>
    * f0 -> ( PostfixExpression() | "++" UnaryExpression() | "--" UnaryExpression() | UnaryOperator() CastExpression() | &lt;SIZEOF&gt; ( UnaryExpression() | "(" TypeSpecifier() ")" ) )
    * </PRE>
    */
   public R visit(UnaryExpression n) {
      R _ret=null;
      n.f0.accept(this);
      return _ret;
   }

   /**
    * <PRE>
    * f0 -> ( "&" | "*" | "+" | "-" | "~" | "!" )
    * </PRE>
    */
   public R visit(UnaryOperator n) {
      R _ret=null;
      n.f0.accept(this);
      return _ret;
   }

   /**
    * <PRE>
    * f0 -> PrimaryExpression()
    * f1 -> ( "[" Expression() "]" | "(" [ ArgumentExpressionList() ] ")" | "." &lt;IDENTIFIER&gt; | "-&gt;" &lt;IDENTIFIER&gt; | "++" | "--" )*
    * </PRE>
    */
   public R visit(PostfixExpression n) {
      R _ret=null;
      n.f0.accept(this);
      n.f1.accept(this);
      return _ret;
   }

   /**
    * <PRE>
    * f0 -> &lt;IDENTIFIER&gt;
    *       | Constant()
    *       | "(" Expression() ")"
    * </PRE>
    */
   public R visit(PrimaryExpression n) {
      R _ret=null;
      n.f0.accept(this);
      return _ret;
   }

   /**
    * <PRE>
    * f0 -> AssignmentOrTypeExpression()
    * f1 -> ( "," AssignmentOrTypeExpression() )*
    * </PRE>
    */
   public R visit(ArgumentExpressionList n) {
      R _ret=null;
      n.f0.accept(this);
      n.f1.accept(this);
      return _ret;
   }

   /**
    * <PRE>
    * f0 -> NoIdentifierTypeSpecifier()
    *       | AssignmentExpression()
    * </PRE>
    */
   public R visit(AssignmentOrTypeExpression n) {
      R _ret=null;
      n.f0.accept(this);
      return _ret;
   }

   /**
    * <PRE>
    * f0 -> &lt;INTEGER_LITERAL&gt;
    *       | &lt;FLOATING_POINT_LITERAL&gt;
    *       | &lt;CHARACTER_LITERAL&gt;
    *       | &lt;STRING_LITERAL&gt;
    * </PRE>
    */
   public R visit(Constant n) {
      R _ret=null;
      n.f0.accept(this);
      return _ret;
   }

}
