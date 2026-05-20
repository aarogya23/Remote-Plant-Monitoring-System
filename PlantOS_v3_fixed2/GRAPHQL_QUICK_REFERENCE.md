# GraphQL API - Quick Reference

## Endpoints
| Endpoint | Purpose |
|----------|---------|
| `POST /graphql` | GraphQL API endpoint |
| `GET /graphiql` | Interactive GraphQL IDE |

## Essential Queries

| Query | Purpose |
|-------|---------|
| `latestPHAndSoilReading` | Get latest pH & soil data |
| `readingsByPh(minPh, maxPh)` | Filter by pH range |
| `readingsBySoil(minSoil, maxSoil)` | Filter by soil moisture |
| `latestReadings(limit)` | Get last N readings |
| `sensorReading(id)` | Get reading by ID |

## Essential Mutations

| Mutation | Purpose |
|----------|---------|
| `addSensorReading(input)` | Add single reading |
| `addMultipleSensorReadings(readings)` | Add multiple readings |

## Key Fields
- **ph**: pH value (0-14)
- **soil**: Soil moisture (0-100)
- **temp**: Temperature (°C)
- **humidity**: Humidity (%)
- **batPct**: Battery percentage
- **health**: Plant health status

## Example - Get Latest pH & Soil in 2 Lines
```graphql
{ latestPHAndSoilReading { ts ph soil health } }
```

## Example - Add Reading in 2 Lines
```graphql
mutation { addSensorReading(input: {temp:25 humidity:60 soil:75 ph:6.8 lux:500 batV:4.0 batPct:90 health:88}) { id ts } }
```
