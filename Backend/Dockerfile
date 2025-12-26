FROM python:3.11-slim
WORKDIR /app
ENV PYTHONPATH=/app 
COPY requirements.txt .
RUN pip install -r requirements.txt
COPY . .
EXPOSE 5000
CMD ["python", "-m", "app.main"]
