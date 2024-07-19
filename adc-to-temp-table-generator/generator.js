const ADC_RESISTOR_IN_OHMS = 8660;
const ADC_MAX_VALUE_F = 4095;

const TABLE_STEP = 5;
const ZERO_CELSIUS_IN_KELVIN = 273.15;

// curve 8404 for R25 = 100k
const curve8404 = [
  { T: 0.0, R: 333960, alpha: 5.2 },
  { T: 5.0, R: 258500, alpha: 5.0 },
  { T: 10.0, R: 201660, alpha: 4.9 },
  { T: 15.0, R: 158500, alpha: 4.7 },
  { T: 20.0, R: 125470, alpha: 4.6 },
  { T: 25.0, R: 100000, alpha: 4.5 },
  { T: 30.0, R: 80223, alpha: 4.3 },
  { T: 35.0, R: 64759, alpha: 4.2 },
  { T: 40.0, R: 52589, alpha: 4.1 },
  { T: 45.0, R: 42951, alpha: 4.0 },
  { T: 50.0, R: 35272, alpha: 3.9 },
  { T: 55.0, R: 29119, alpha: 3.8 },
  { T: 60.0, R: 24161, alpha: 3.7 },
  { T: 65.0, R: 20144, alpha: 3.6 },
  { T: 70.0, R: 16874, alpha: 3.5 },
  { T: 75.0, R: 14198, alpha: 3.4 },
  { T: 80.0, R: 11998, alpha: 3.3 },
  { T: 85.0, R: 10181, alpha: 3.2 },
  { T: 90.0, R: 8674.4, alpha: 3.2 },
  { T: 95.0, R: 7418.9, alpha: 3.1 },
  { T: 100.0, R: 6368.8, alpha: 3.0 },
  { T: 105.0, R: 5487, alpha: 2.9 },
  { T: 110.0, R: 4743.7, alpha: 2.9 },
  { T: 115.0, R: 4114.8, alpha: 2.8 },
  { T: 120.0, R: 3580.8, alpha: 2.7 },
  { T: 125.0, R: 3125.9, alpha: 2.7 },
  { T: 130.0, R: 2737, alpha: 2.6 },
  { T: 135.0, R: 2403.5, alpha: 2.6 },
  { T: 140.0, R: 2116.7, alpha: 2.5 },
  { T: 145.0, R: 1869.1, alpha: 2.5 },
  { T: 150.0, R: 1654.9, alpha: 2.4 },
  { T: 155.0, R: 1469.1, alpha: 2.4 },
  { T: 160.0, R: 1307.3, alpha: 2.3 },
  { T: 165.0, R: 1166.2, alpha: 2.3 },
  { T: 170.0, R: 1042.7, alpha: 2.2 },
  { T: 175.0, R: 934.46, alpha: 2.2 },
  { T: 180.0, R: 839.28, alpha: 2.1 },
  { T: 185.0, R: 755.42, alpha: 2.1 },
  { T: 190.0, R: 681.35, alpha: 2.0 },
  { T: 195.0, R: 615.78, alpha: 2.0 },
  { T: 200.0, R: 557.62, alpha: 2.0 },
  { T: 205.0, R: 505.92, alpha: 1.9 },
  { T: 210.0, R: 459.86, alpha: 1.9 },
  { T: 215.0, R: 418.76, alpha: 1.9 },
  { T: 220.0, R: 381.99, alpha: 1.8 },
  { T: 225.0, R: 349.06, alpha: 1.8 },
  { T: 230.0, R: 319.49, alpha: 1.8 },
  { T: 235.0, R: 292.9, alpha: 1.7 },
  { T: 240.0, R: 268.95, alpha: 1.7 },
  { T: 245.0, R: 247.34, alpha: 1.7 },
  { T: 250.0, R: 227.8, alpha: 1.6 },
];

(function main() {
  const resistanceTable = [];
  for (const { T, R, alpha } of curve8404) {
    resistanceTable.push({ T, R });
    for (let tempDelta = 1; tempDelta < TABLE_STEP; ++tempDelta) {
      const targetTemp = T + tempDelta;
      const targetTempInKelvins = targetTemp + ZERO_CELSIUS_IN_KELVIN;
      const baseTempInKelvins = T + ZERO_CELSIUS_IN_KELVIN;
      const exp = (alpha / 100) * (baseTempInKelvins ** 2) * ((1 / targetTempInKelvins) - (1 / baseTempInKelvins));
      const resistance = R * Math.pow(Math.E, exp);
      resistanceTable.push({ T: targetTemp, R: resistance });
    }
  }

  const adcTable = resistanceTable.map(({ T, R }) => ({
    T,
    adcValue: Math.round((R / (R + ADC_RESISTOR_IN_OHMS)) * ADC_MAX_VALUE_F),
  }));

  console.log(adcTable.reverse().map(({ T, adcValue }) => `  { ${adcValue}, ${T} },`).join('\n'));
}());