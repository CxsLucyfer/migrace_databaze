/* Soubor Kap09\03\konvertor\Prevod.java

 * UPRAVEN� VERZE VYU��VAJ�C� V�JIMEK, PODRUH�
 * tentokr�t maj� r�zn� v�jimky r�zn� handlery
 * a nav�c pou�ijeme koncovku k vyps�n� pou�en�, jak program spou�t�t
 
 * P�evod des�tkov�ho cel�ho ��sla do jin� ��seln� soustavy
 * program se spou�t� p��kazem
        java Prevod ��slo z�klad
 * tedy nap�.
	java Prevod 8 2
 * kdy� chceme p�ev�st ��slo 8 do dvojkov� soustavy.
 * Pak vyp�e 1000, co� je z�pis 8 ve dvojkov� soustav�.

 * Vyu��v� slu�em t��dy Konvertor
 */

public class Prevod {

  public Prevod() {
  }
  public static void main(String[] args) {
    try{
      int cislo = Integer.parseInt(args[0]);
      int zaklad = Integer.parseInt(args[1]);
      System.out.println(new konvertor.Konvertor(zaklad).konverze(cislo));
    }
    catch(ArrayIndexOutOfBoundsException e)
    {
        System.out.println("Pri volani je treba zadat dva parametry");
    }
    catch(NumberFormatException e){
      System.out.println("Jeden z parametru neni cislo " + e.getMessage());
    }
    catch(konvertor.WrongBaseException e){
      System.out.println("Zaklad " + e.getMessage() + " lezi mimo interval 2--36");
    }
    finally{
        System.out.println("Pouziti:\n"+
              "java Prevod cislo cilova_soustava\n"+
              "cilova soustava musi byt v rozmezi 2 - 36\n");
    }
  }
}