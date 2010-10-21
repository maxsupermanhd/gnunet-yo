//
// Generated by JTB 1.3.2
//

package org.gnunet.seaspider.parser.visitors;
import org.gnunet.seaspider.parser.nodes.*;

import java.util.*;

/**
 * All GJ visitors with no argument must implement this interface.
 */

public interface GJNoArguVisitor<R> {

   //
   // GJ Auto class visitors with no argument
   //

   public R visit(NodeList n);
   public R visit(NodeListOptional n);
   public R visit(NodeOptional n);
   public R visit(NodeSequence n);
   public R visit(NodeToken n);

   //
   // User-generated visitor methods below
   //

   /**
    * <PRE>
    * f0 -> ( ExternalDeclaration() )+
    * </PRE>
    */
   public R visit(TranslationUnit n);

   /**
    * <PRE>
    * f0 -> ( StorageClassSpecifier() )*
    * f1 -> ( FunctionDeclaration() | StructOrUnionSpecifier() | VariableDeclaration() | TypeDeclaration() )
    * </PRE>
    */
   public R visit(ExternalDeclaration n);

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
   public R visit(FunctionDeclaration n);

   /**
    * <PRE>
    * f0 -> ( &lt;STATIC&gt; | &lt;EXTERN&gt; )
    * </PRE>
    */
   public R visit(StorageClassSpecifier n);

   /**
    * <PRE>
    * f0 -> &lt;TYPEDEF&gt;
    * f1 -> ( DataType() | FunctionType() )
    * f2 -> ";"
    * </PRE>
    */
   public R visit(TypeDeclaration n);

   /**
    * <PRE>
    * f0 -> StructOrUnionSpecifier()
    * f1 -> &lt;IDENTIFIER&gt;
    * </PRE>
    */
   public R visit(DataType n);

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
   public R visit(FunctionType n);

   /**
    * <PRE>
    * f0 -> ParameterDeclaration()
    * f1 -> ( "," ParameterDeclaration() )*
    * f2 -> [ "," "..." ]
    * </PRE>
    */
   public R visit(ParameterList n);

   /**
    * <PRE>
    * f0 -> TypeSpecifier()
    * f1 -> &lt;IDENTIFIER&gt;
    * f2 -> [ Array() ]
    * </PRE>
    */
   public R visit(ParameterDeclaration n);

   /**
    * <PRE>
    * f0 -> VariableClassSpecifier()
    * f1 -> TypeSpecifier()
    * f2 -> InitDeclaratorList()
    * f3 -> ";"
    * </PRE>
    */
   public R visit(VariableDeclaration n);

   /**
    * <PRE>
    * f0 -> [ &lt;STATIC&gt; ]
    * f1 -> VariableDeclaration()
    * </PRE>
    */
   public R visit(LocalVariableDeclaration n);

   /**
    * <PRE>
    * f0 -> ( &lt;AUTO&gt; | &lt;REGISTER&gt; )*
    * </PRE>
    */
   public R visit(VariableClassSpecifier n);

   /**
    * <PRE>
    * f0 -> [ &lt;CONST&gt; ]
    * f1 -> ( &lt;VOID&gt; | &lt;CHAR&gt; | &lt;SHORT&gt; [ &lt;INT&gt; ] | &lt;INT&gt; | &lt;LONG&gt; [ &lt;LONG&gt; ] | &lt;FLOAT&gt; | &lt;DOUBLE&gt; | ( &lt;SIGNED&gt; | &lt;UNSIGNED&gt; ) [ &lt;CHAR&gt; | &lt;SHORT&gt; [ &lt;INT&gt; ] | &lt;INT&gt; | &lt;LONG&gt; [ &lt;LONG&gt; ] ] | StructOrUnionSpecifier() | EnumSpecifier() | &lt;IDENTIFIER&gt; )
    * f2 -> [ Pointer() ]
    * f3 -> [ Array() ]
    * </PRE>
    */
   public R visit(TypeSpecifier n);

   /**
    * <PRE>
    * f0 -> [ &lt;CONST&gt; ]
    * f1 -> ( &lt;VOID&gt; | &lt;CHAR&gt; | &lt;SHORT&gt; [ &lt;INT&gt; ] | &lt;INT&gt; | &lt;LONG&gt; [ &lt;LONG&gt; ] | &lt;FLOAT&gt; | &lt;DOUBLE&gt; | ( &lt;SIGNED&gt; | &lt;UNSIGNED&gt; ) [ &lt;CHAR&gt; | &lt;SHORT&gt; [ &lt;INT&gt; ] | &lt;INT&gt; | &lt;LONG&gt; [ &lt;LONG&gt; ] ] | StructOrUnionSpecifier() | EnumSpecifier() )
    * f2 -> [ Pointer() ]
    * f3 -> [ Array() ]
    * </PRE>
    */
   public R visit(NoIdentifierTypeSpecifier n);

   /**
    * <PRE>
    * f0 -> StructOrUnion() [ &lt;IDENTIFIER&gt; ] "{" StructDeclarationList() "}"
    *       | StructOrUnion() &lt;IDENTIFIER&gt;
    * </PRE>
    */
   public R visit(StructOrUnionSpecifier n);

   /**
    * <PRE>
    * f0 -> ( &lt;STRUCT&gt; | &lt;UNION&gt; )
    * </PRE>
    */
   public R visit(StructOrUnion n);

   /**
    * <PRE>
    * f0 -> ( StructDeclaration() )+
    * </PRE>
    */
   public R visit(StructDeclarationList n);

   /**
    * <PRE>
    * f0 -> InitDeclarator()
    * f1 -> ( "," InitDeclarator() )*
    * </PRE>
    */
   public R visit(InitDeclaratorList n);

   /**
    * <PRE>
    * f0 -> &lt;IDENTIFIER&gt;
    * f1 -> [ Array() ]
    * f2 -> [ "=" Initializer() ]
    * </PRE>
    */
   public R visit(InitDeclarator n);

   /**
    * <PRE>
    * f0 -> TypeSpecifier()
    * f1 -> &lt;IDENTIFIER&gt;
    * f2 -> [ Array() | ":" ConstantExpression() ]
    * f3 -> [ &lt;IDENTIFIER&gt; ]
    * f4 -> ";"
    * </PRE>
    */
   public R visit(StructDeclaration n);

   /**
    * <PRE>
    * f0 -> &lt;ENUM&gt;
    * f1 -> ( [ &lt;IDENTIFIER&gt; ] "{" EnumeratorList() "}" | &lt;IDENTIFIER&gt; )
    * </PRE>
    */
   public R visit(EnumSpecifier n);

   /**
    * <PRE>
    * f0 -> Enumerator()
    * f1 -> ( "," Enumerator() )*
    * </PRE>
    */
   public R visit(EnumeratorList n);

   /**
    * <PRE>
    * f0 -> &lt;IDENTIFIER&gt;
    * f1 -> [ "=" ConstantExpression() ]
    * </PRE>
    */
   public R visit(Enumerator n);

   /**
    * <PRE>
    * f0 -> "*"
    * f1 -> [ &lt;CONST&gt; ]
    * f2 -> [ Pointer() ]
    * </PRE>
    */
   public R visit(Pointer n);

   /**
    * <PRE>
    * f0 -> &lt;IDENTIFIER&gt;
    * f1 -> ( "," &lt;IDENTIFIER&gt; )*
    * </PRE>
    */
   public R visit(IdentifierList n);

   /**
    * <PRE>
    * f0 -> ( AssignmentExpression() | "{" InitializerList() [ "," ] "}" )
    * </PRE>
    */
   public R visit(Initializer n);

   /**
    * <PRE>
    * f0 -> Initializer()
    * f1 -> ( "," Initializer() )*
    * </PRE>
    */
   public R visit(InitializerList n);

   /**
    * <PRE>
    * f0 -> "["
    * f1 -> [ ConstantExpression() ]
    * f2 -> "]"
    * </PRE>
    */
   public R visit(Array n);

   /**
    * <PRE>
    * f0 -> ( LabeledStatement() | ExpressionStatement() | CompoundStatement() | SelectionStatement() | IterationStatement() | JumpStatement() )
    * </PRE>
    */
   public R visit(Statement n);

   /**
    * <PRE>
    * f0 -> ( &lt;IDENTIFIER&gt; ":" Statement() | &lt;CASE&gt; ConstantExpression() ":" Statement() | &lt;DFLT&gt; ":" Statement() )
    * </PRE>
    */
   public R visit(LabeledStatement n);

   /**
    * <PRE>
    * f0 -> [ Expression() ]
    * f1 -> ";"
    * </PRE>
    */
   public R visit(ExpressionStatement n);

   /**
    * <PRE>
    * f0 -> "{"
    * f1 -> ( LocalVariableDeclaration() | Statement() )*
    * f2 -> "}"
    * </PRE>
    */
   public R visit(CompoundStatement n);

   /**
    * <PRE>
    * f0 -> ( &lt;IF&gt; "(" Expression() ")" Statement() [ &lt;ELSE&gt; Statement() ] | &lt;SWITCH&gt; "(" Expression() ")" Statement() )
    * </PRE>
    */
   public R visit(SelectionStatement n);

   /**
    * <PRE>
    * f0 -> ( &lt;WHILE&gt; "(" Expression() ")" Statement() | &lt;DO&gt; Statement() &lt;WHILE&gt; "(" Expression() ")" ";" | &lt;FOR&gt; "(" [ Expression() ] ";" [ Expression() ] ";" [ Expression() ] ")" Statement() )
    * </PRE>
    */
   public R visit(IterationStatement n);

   /**
    * <PRE>
    * f0 -> ( &lt;GOTO&gt; &lt;IDENTIFIER&gt; ";" | &lt;CONTINUE&gt; ";" | &lt;BREAK&gt; ";" | &lt;RETURN&gt; [ Expression() ] ";" )
    * </PRE>
    */
   public R visit(JumpStatement n);

   /**
    * <PRE>
    * f0 -> AssignmentExpression()
    * f1 -> ( "," AssignmentExpression() )*
    * </PRE>
    */
   public R visit(Expression n);

   /**
    * <PRE>
    * f0 -> UnaryExpression() AssignmentOperator() AssignmentExpression()
    *       | ConditionalExpression()
    * </PRE>
    */
   public R visit(AssignmentExpression n);

   /**
    * <PRE>
    * f0 -> ( "=" | "*=" | "/=" | "%=" | "+=" | "-=" | "&lt;&lt;=" | "&gt;&gt;=" | "&=" | "^=" | "|=" )
    * </PRE>
    */
   public R visit(AssignmentOperator n);

   /**
    * <PRE>
    * f0 -> LogicalORExpression()
    * f1 -> [ "?" Expression() ":" ConditionalExpression() ]
    * </PRE>
    */
   public R visit(ConditionalExpression n);

   /**
    * <PRE>
    * f0 -> ConditionalExpression()
    * </PRE>
    */
   public R visit(ConstantExpression n);

   /**
    * <PRE>
    * f0 -> LogicalANDExpression()
    * f1 -> [ "||" LogicalORExpression() ]
    * </PRE>
    */
   public R visit(LogicalORExpression n);

   /**
    * <PRE>
    * f0 -> InclusiveORExpression()
    * f1 -> [ "&&" LogicalANDExpression() ]
    * </PRE>
    */
   public R visit(LogicalANDExpression n);

   /**
    * <PRE>
    * f0 -> ExclusiveORExpression()
    * f1 -> [ "|" InclusiveORExpression() ]
    * </PRE>
    */
   public R visit(InclusiveORExpression n);

   /**
    * <PRE>
    * f0 -> ANDExpression()
    * f1 -> [ "^" ExclusiveORExpression() ]
    * </PRE>
    */
   public R visit(ExclusiveORExpression n);

   /**
    * <PRE>
    * f0 -> EqualityExpression()
    * f1 -> [ "&" ANDExpression() ]
    * </PRE>
    */
   public R visit(ANDExpression n);

   /**
    * <PRE>
    * f0 -> RelationalExpression()
    * f1 -> [ ( "==" | "!=" ) EqualityExpression() ]
    * </PRE>
    */
   public R visit(EqualityExpression n);

   /**
    * <PRE>
    * f0 -> ShiftExpression()
    * f1 -> [ ( "&lt;" | "&gt;" | "&lt;=" | "&gt;=" ) RelationalExpression() ]
    * </PRE>
    */
   public R visit(RelationalExpression n);

   /**
    * <PRE>
    * f0 -> AdditiveExpression()
    * f1 -> [ ( "&lt;&lt;" | "&gt;&gt;" ) ShiftExpression() ]
    * </PRE>
    */
   public R visit(ShiftExpression n);

   /**
    * <PRE>
    * f0 -> MultiplicativeExpression()
    * f1 -> [ ( "+" | "-" ) AdditiveExpression() ]
    * </PRE>
    */
   public R visit(AdditiveExpression n);

   /**
    * <PRE>
    * f0 -> CastExpression()
    * f1 -> [ ( "*" | "/" | "%" ) MultiplicativeExpression() ]
    * </PRE>
    */
   public R visit(MultiplicativeExpression n);

   /**
    * <PRE>
    * f0 -> ( "(" TypeSpecifier() ")" CastExpression() | UnaryExpression() )
    * </PRE>
    */
   public R visit(CastExpression n);

   /**
    * <PRE>
    * f0 -> ( PostfixExpression() | "++" UnaryExpression() | "--" UnaryExpression() | UnaryOperator() CastExpression() | &lt;SIZEOF&gt; ( UnaryExpression() | "(" TypeSpecifier() ")" ) )
    * </PRE>
    */
   public R visit(UnaryExpression n);

   /**
    * <PRE>
    * f0 -> ( "&" | "*" | "+" | "-" | "~" | "!" )
    * </PRE>
    */
   public R visit(UnaryOperator n);

   /**
    * <PRE>
    * f0 -> PrimaryExpression()
    * f1 -> ( "[" Expression() "]" | "(" [ ArgumentExpressionList() ] ")" | "." &lt;IDENTIFIER&gt; | "-&gt;" &lt;IDENTIFIER&gt; | "++" | "--" )*
    * </PRE>
    */
   public R visit(PostfixExpression n);

   /**
    * <PRE>
    * f0 -> &lt;IDENTIFIER&gt;
    *       | Constant()
    *       | "(" Expression() ")"
    * </PRE>
    */
   public R visit(PrimaryExpression n);

   /**
    * <PRE>
    * f0 -> AssignmentOrTypeExpression()
    * f1 -> ( "," AssignmentOrTypeExpression() )*
    * </PRE>
    */
   public R visit(ArgumentExpressionList n);

   /**
    * <PRE>
    * f0 -> NoIdentifierTypeSpecifier()
    *       | AssignmentExpression()
    * </PRE>
    */
   public R visit(AssignmentOrTypeExpression n);

   /**
    * <PRE>
    * f0 -> &lt;INTEGER_LITERAL&gt;
    *       | &lt;FLOATING_POINT_LITERAL&gt;
    *       | &lt;CHARACTER_LITERAL&gt;
    *       | &lt;STRING_LITERAL&gt;
    * </PRE>
    */
   public R visit(Constant n);

}

