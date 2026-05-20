# GraphQL API Documentation - Plant Monitoring System

## Overview
The Plant Monitoring System now includes a **GraphQL API** for accessing and managing sensor data including pH value and soil moisture readings. The API provides a flexible and efficient way to query and mutate sensor data.

## Getting Started

### Access GraphQL
- **GraphQL Endpoint**: `http://localhost:8080/graphql`
- **GraphiQL Interface** (Interactive IDE): `http://localhost:8080/graphiql`

Once the application is running, open `http://localhost:8080/graphiql` in your browser to interact with the API using the GraphiQL interface.

---

## Schema

### Types

#### SensorReading
```graphql
type SensorReading {
    id: Long!
    ts: String!              # Timestamp when reading was recorded
    temp: Float!             # Temperature in Celsius
    humidity: Float!         # Humidity percentage
    soil: Int!               # Soil moisture level (0-100)
    ph: Float!               # pH value (0-14)
    lux: Float!              # Light intensity
    batV: Float!             # Battery voltage
    batPct: Int!             # Battery percentage
    health: Int!             # Plant health status
}
```

#### SensorReadingInput
```graphql
input SensorReadingInput {
    temp: Float!
    humidity: Float!
    soil: Int!
    ph: Float!
    lux: Float!
    batV: Float!
    batPct: Int!
    health: Int!
}
```

---

## Queries

### 1. Get All Sensor Readings
```graphql
query {
  allSensorReadings(limit: 10) {
    id
    ts
    temp
    humidity
    soil
    ph
    lux
    batV
    batPct
    health
  }
}
```

### 2. Get Sensor Reading by ID
```graphql
query {
  sensorReading(id: 1) {
    id
    ts
    temp
    humidity
    soil
    ph
    lux
    batV
    batPct
    health
  }
}
```

### 3. Get Latest Readings
```graphql
query {
  latestReadings(limit: 5) {
    id
    ts
    temp
    humidity
    soil
    ph
  }
}
```

### 4. Get Readings by pH Range
```graphql
query {
  readingsByPh(minPh: 5.5, maxPh: 7.5) {
    id
    ts
    ph
    temp
    humidity
  }
}
```

**Example**: pH between 6.0 and 7.5 (neutral range)
```graphql
query {
  readingsByPh(minPh: 6.0, maxPh: 7.5) {
    id
    ts
    ph
    soil
    health
  }
}
```

### 5. Get Readings by Soil Moisture Range
```graphql
query {
  readingsBySoil(minSoil: 40, maxSoil: 80) {
    id
    ts
    soil
    temp
    humidity
  }
}
```

**Example**: Soil moisture between 50-80 (optimal range)
```graphql
query {
  readingsBySoil(minSoil: 50, maxSoil: 80) {
    id
    ts
    soil
    ph
    health
  }
}
```

### 6. Get Latest pH and Soil Moisture Reading
```graphql
query {
  latestPHAndSoilReading {
    id
    ts
    ph
    soil
    temp
    humidity
    health
  }
}
```

---

## Mutations

### 1. Add Single Sensor Reading
```graphql
mutation {
  addSensorReading(input: {
    temp: 24.5
    humidity: 65.0
    soil: 75
    ph: 6.8
    lux: 500.0
    batV: 4.2
    batPct: 85
    health: 90
  }) {
    id
    ts
    temp
    humidity
    soil
    ph
    lux
    batV
    batPct
    health
  }
}
```

**Example**: Adding a sensor reading with optimal pH (6.8) and good soil moisture (75)
```graphql
mutation {
  addSensorReading(input: {
    temp: 22.0
    humidity: 70.0
    soil: 75
    ph: 6.8
    lux: 450.0
    batV: 4.1
    batPct: 88
    health: 95
  }) {
    id
    ts
    ph
    soil
    health
  }
}
```

### 2. Add Multiple Sensor Readings
```graphql
mutation {
  addMultipleSensorReadings(readings: [
    {
      temp: 24.5
      humidity: 65.0
      soil: 75
      ph: 6.8
      lux: 500.0
      batV: 4.2
      batPct: 85
      health: 90
    },
    {
      temp: 23.2
      humidity: 68.0
      soil: 72
      ph: 6.9
      lux: 480.0
      batV: 4.1
      batPct: 82
      health: 92
    }
  ]) {
    id
    ts
    soil
    ph
    health
  }
}
```

---

## Common Use Cases

### Monitor pH Levels
```graphql
query {
  readingsByPh(minPh: 5.0, maxPh: 8.0) {
    ts
    ph
    health
  }
}
```

### Check Plant Hydration
```graphql
query {
  latestReadings(limit: 1) {
    soil
    humidity
    health
  }
}
```

### Get Recent Critical Readings
```graphql
query {
  readingsBySoil(minSoil: 20, maxSoil: 40) {
    ts
    soil
    ph
    health
  }
}
```

### Log Sensor Data
```graphql
mutation {
  addSensorReading(input: {
    temp: 25.0
    humidity: 60.0
    soil: 70
    ph: 7.0
    lux: 550.0
    batV: 4.0
    batPct: 90
    health: 88
  }) {
    id
    ts
  }
}
```

---

## Tips

1. **Use Pagination**: Always limit results using the `limit` parameter for better performance
2. **Filter by pH**: Ideal pH ranges:
   - **Most plants**: 6.0 - 7.5
   - **Acid-loving plants**: 4.5 - 6.0
   - **Alkaline-loving plants**: 7.0 - 8.5

3. **Soil Moisture Levels**:
   - **Dry**: 0 - 30
   - **Moderate**: 40 - 60
   - **Wet**: 70 - 100

4. **Use GraphiQL**: Open `http://localhost:8080/graphiql` for interactive query building with autocomplete

---

## REST Alternative
The system also supports the original REST endpoint:
- **POST** `/api/data` - Submit sensor data (JSON format)

---

## Error Handling
If a query fails, GraphQL will return error details in the response. Check the GraphiQL interface for validation errors and schema information.
