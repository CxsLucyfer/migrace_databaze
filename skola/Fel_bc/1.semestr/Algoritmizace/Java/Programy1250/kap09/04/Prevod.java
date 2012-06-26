/* Soubor Kap09\04\Prevod.java
 * P�evod des�tkov�ho cel�ho ��sla do jin� ��seln� soustavy
 * program se spou�t� p��kazem 
        java Prevod ��slo z�klad
 * tedy nap�.
	java Prevod 8 2
 * kdy� chceme p�ev�st ��slo 8 do dvojko� soustavy. 
 * Pak vyp�e 1000, co� je z�pis 8 ve dvojkov� soustav�.

 * Vyu��v� slu�eb t��dy Konvertor, kterou najdete v Kap06\01\Konvertor.java
 */

public class Prevod {

  public Prevod() {
  }
  public static void main(String[] args) {
    if(args.length != 2)
    {
      System.out.println("Pouziti:\n"+
            "java Prevod cislo cilova_soustava\n"+
            "cilova soustava musi byt v rozmezi 2 - 36");
      System.exit(1);
    }
    int cislo = Integer.parseInt(args[0]);
    int zaklad = Integer.parseInt(args[1]);
    System.out.println(new Konvertor(zaklad).konverze(cislo));
  }
}