module namespace mylib="http://mylib";

declare function mylib:dalsi( $prvek as element() ) as xs:integer
{
let $pocet = 0
if ( $prvek/@next= "END")
then return $pocet
else 
	return mylib:dalsi( $prvek/@value=$prvek/@next ) + 1

};
	
(: Mnel by vracet posledni prvek v listu, nepodarilo se rozchodit :)
(:

if then else
1 dotaz mus� vyu��t deklaraci u�ivatelsk� funkce pro rekurzivn� pr�chod XML dokumentem (viz. p��klad 9 ze cvi�en�)

:)