/* Soubor Kap08\01\Switch1.java
 * pou�it� p��kazu switch - prvn�, nepoda�en� pokus
 * Program si vy��d� mal� cel� ��slo a podle jeho hodnoty vyp�e pozdrav
*/

public class Switch1 {
  public static void main(String[] s){
    System.out.print("Zadej male cele cislo: ");
    int i = MojeIO.inInt();

    switch(i)
    {
      case 1: 
      case 5: 
	System.out.println("Ahoj");
      case 2:
      case 6: 
	System.out.println("Te pic");
      case 3:
      case 7: 
	System.out.println("Cau");
      default: 
	System.out.println("Nerozumim");
    }
  }
}