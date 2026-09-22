# 5.1 The grammar itself
## 5.1.0 Accepted syntax
In relation defintions and queries, white spaces are ignored, unless within a string.
Strings can be written without quotes, but quotes are needed if you want to include:
- a comma (,),
- a period (.),
- a space ( ),
- a semicolon (;),
- a parantheses (()),
- a brace ({}),
- a single quote character*  ('') (have to use two single quotes right next to eachother to represent one)

Newlines are also ignored, except when completing an operation, where once a period (.) is inserted, newline will submit the query up to that point and cut off any following text
Comments are any text wrapped between /* and */, their contents are ignored when the query/relation is executed

## 5.1.1 Relation definitions
A relation contains a name, a schema (list of attributes which are name + index + type (STRING or NUMBER)), and a list of tuples (which follow the schema)
Relations can be defined with or without defaults values
Since relations are sets duplicate tuples get ignored

### Without any values
```cpp
Herbavoires (DID = NUMBER, Species = STRING, Extinct = NUMBER, Height = NUMBER, Name = STRING) = {}.
```


### With tuples
```cpp
Herbavoires (DID = NUMBER, Species = STRING, Extinct = NUMBER, Height = NUMBER, Name = STRING) = {
    1, Tricerotops, 1, 6.2, Tom;
    1, Tricerotops, 1, 6.2, Tom;
    // Cool comment
    2, Brachiosaurus, 1, 5.5, Michael; 
    3, 'Brown Chicken', 0, 1.2, Colin;
}.
```

The relation definitions above gets compacted down to the one below once whitespaces, comments, duplicates, and extra newlines are ignored

### Without any values
```cpp
Herbavoires(DID=NUMBER,Species=STRING,Extinct=NUMBER,Height=NUMBER,Name=STRING)={}.
```

### With tuples
```cpp
Herbavoires(DID=NUMBER,Species=STRING,Extinct=NUMBER,Height=NUMBER,Name=STRING)={1,Tricerotops,1,6.2,Tom;2,Brachiosaurus,1,5.5,Michael;3,'Brown Chicken',0,1.2,Colin;}.
```

## 5.1.2
The following REX (Relational Expressions) are possible to make in your queries.  A REX is either a relation itself or an expression that returns a relation

### Unary
select{<condition>}(<REX>) -> returns a relation, with the the same schema as the input relation, but only the tuples witch match the condition 
project{<attribute-list>}(<REX>) -> returns a relation, with the the same tuples as the input relation, but only the attributes specified by the attribute list 
rename{<new-relation-name>](<REX>) -> returns a relation, with the the same attributes and tuples as the input relation, but with a new relation name
rename{<new-attribute-name>, <old-attribute-name>}(<REX>) -> returns a relation, with the the tuples as the input relation, but with one attribute's name changed (and only its name)

### Binary
(<REX>)union(<REX>) -> returns a relation, with all of the tuples from both input relations (duplicates collapse).  The relations must have the exact same schema
(<REX>)intersect(<REX>) -> returns a relation, with only the tuples that exist in both input relations.  The relations must have the exact same schema
(<REX>)minus(<REX>) -> returns a relation, with only the tuples that exist in the first relation but not in the second.  The relations must have the exact same schema
(<REX>)times(<REX>) -> returns a relation, with the schema of both relations together (A.name, A.colour, B.name, B.type {where A and B are relation names}), and every tuple of each relation matched with every tuple of the other relation
(<REX>)join{<condition>}(<REX>) -> returns a relation, performs "times" of the two relations, but then filters the rows based on a "select" condition

Conditions, attribute names/lists, and relation names are all written in braces {}.
sub-REXs are always written within parantheses ().
Relational algebra operations are not case sensitive, SELECT is the same as select is the same as sEleCT

Conditions are comparisons combined with "AND", "NOT", "OR", and "()".
A comparison has two operands (which is either a REX or another comparison) and one of the operators '=', '!=', '<', '<=', '>', or '>=' between them.

# 5.2 Precedence and associativity
All binary operators have the same precedence
Everything is left associative so in (A union B minus C), A union B -> X happens, then X minus C happens
Precedence can be overruled by parantheses, the innermost parantheses are evaluated first, etc. So in (A union (B minus C)), B minus C -> X happens, then A union X happens

# 5.3 An ambiguity demonstration
Expr ::= Expr "union" Expr
       | Expr "minus" Expr
       | "(" Expr ")"
       | IDENT

A union B minus C
Can be evaluated into the parse trees
         Expr
       /   |   \
    Expr minus Expr
  /  |  \       |
Expr union Expr  C
 |         |
 A         B

         Expr
   /   |   \
Expr union Expr
  |      / | \
 A      /  |  \
      Expr minus Expr
        |         |
        B         C

If A has one tuple ("COOL", "MATH", "GAMES")
and B has one tuple ("COOL", "MATH", "GAMES")
and C has one tuple ("COOL", "MATH", "GAMES")

The first tree will produce an empty relation
The second tree will produce a relation with one tuple ("COOL", "MATH", "GAMES")

My grammar removes the ambiguity because it says the leftmost operation goes first
So only the first parse tree and result will happen
If you want the second one to happen, you have to use parantheses

# 5.4 A parsing strategy justification
# 5.5 The Grammer Itself
