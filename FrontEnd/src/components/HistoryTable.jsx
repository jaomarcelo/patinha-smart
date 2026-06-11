export default function HistoryTable({ historico }) {
  return (
    <div className="card">
      <h2>📊 Histórico</h2>

      <table>
        <thead>
          <tr>
            <th>Horário</th>
            <th>Ração</th>
          </tr>
        </thead>

        <tbody>
          {historico.map((item, index) => (
            <tr key={index}>
              <td>{item.hora}</td>
              <td>{item.racao}%</td>
            </tr>
          ))}
        </tbody>
      </table>
    </div>
  );
}